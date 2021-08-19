/*
  makeobjfile.c

Copyright (c) 1985, '86, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Sun Aug 18 12:18:16 1985
Edit History:
Ed Taft: Sun Dec 17 15:06:30 1989
Ivor Durham: Sat Aug 27 08:42:29 1988
Jim Sandman: Wed Mar 16 15:50:41 1988
Joe Pasqua: Fri Jan 13 15:07:47 1989
End Edit History.

Implementation of makeobjfile.h for BSD 4.* Unix and derivatives.

Restrictions:
  - OMAGIC files only
  - No data segments
  - No Adobe multi-segment files
  - No entry point or relocation information
  - Uses static storage -- only one file at a time can be created
*/

#include PACKAGE_SPECS
#include ENVIRONMENT
#include PSLIB
#include PUBLICTYPES
#include STREAM
#include UNIXSTREAM

#include "makeobjfile.h"
#include "vm_reverse.h"
#include <a.out.h>

#if CANREVERSEVM
#define XL_OMAGIC 01407
  /* generate little-endian object file for Weitek XL-80xx consisting
     of data instead of text segment */
#endif CANREVERSEVM

typedef struct {		/* storage block for symbols and such */
  char *base;			/* -> base of block */
  long int maxLength;		/* allocated size of block */
  long int length;		/* bytes used in block */
  } Block;

private Stm objStm;
private struct exec header;
private Block *symBlock, *strBlock;

private Block *MakeBlock(/* int maxLength */);
private char *AppendToBlock(/* Block *block; int size */);
private procedure FreeBlock (/* Block *block */);

public int ObjFileBegin(name)
  char *name;
{
  if ((objStm = os_fopen(name, "w")) == NULL) return 0;
  os_bzero((char *) &header, (long int) sizeof(header));
  header.a_magic = OMAGIC;
#if CANREVERSEVM
  if (vISP == isp_xl8000) header.a_magic = XL_OMAGIC;
#endif CANREVERSEVM
  symBlock = MakeBlock((long int) 1024);
  strBlock = MakeBlock((long int) 1024);
  (void) AppendToBlock(strBlock, (long int) 4); /* reserve space for size */
  fwrite((char *)&header, (long int) 1, (long int) sizeof(header), objStm);
  return 1;
}

public int ObjFileEnd()
{
  int result;
  register struct nlist *pSym, *endSym;

  if ((header.a_text & 3) != 0)	/* round up to mod 4 boundary */
    ObjPutText("\0\0\0", (long int) (4 - (header.a_text & 3)));
  header.a_bss = (header.a_bss + 3) & -4;
#if CANREVERSEVM
  if (header.a_magic == XL_OMAGIC)
    {
    header.a_data = header.a_text;
    header.a_text = 0;
    }
#endif CANREVERSEVM

  header.a_syms = symBlock->length;
  *(long int *)(strBlock->base) = strBlock->length;

  /* Now relocate symbols as appropriate */
  pSym = (struct nlist *) symBlock->base;
  endSym = (struct nlist *) (symBlock->base + symBlock->length);
  for ( ; pSym < endSym; pSym++)
    {
    switch (pSym->n_type)
      {
      case N_DATA: case N_DATA+N_EXT:
	pSym->n_value += header.a_text;
	break;
      case N_BSS: case N_BSS+N_EXT:
	pSym->n_value += header.a_text + header.a_data;
	break;
      }
#if CANREVERSEVM
    if (vSWAPBITS != SWAPBITS)
      {
      static Card8 fieldsNlist[] = {32, 8, 8, 16, 32, 0};
      ReverseFields((PCard8) pSym, fieldsNlist);
      }
#endif CANREVERSEVM
    }

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS)
    ReverseFields(strBlock->base, "\040");  /* field descriptor {32} */
#endif CANREVERSEVM

  fwrite(symBlock->base, (long int) 1, symBlock->length, objStm);
  fwrite(strBlock->base, (long int) 1, strBlock->length, objStm);
  os_rewind(objStm);

#if CANREVERSEVM
  if (vSWAPBITS != SWAPBITS)
    {
    static Card8 fieldsExec[] = {32, 32, 32, 32, 32, 32, 32, 32, 0};
    ReverseFields((PCard8) &header, fieldsExec);
    }
#endif CANREVERSEVM

  fwrite((char *)&header, (long int) 1, (long int) sizeof(header), objStm);
  FreeBlock(symBlock);
  FreeBlock(strBlock);
  result = !ferror(objStm);
  return (fclose(objStm) == 0) && result;
}

public procedure ObjPutText(ptr, count)
  char *ptr; long int count;
{
  fwrite(ptr, (long int) 1, count, objStm);
  header.a_text += count;
}

public procedure ObjPutSymbol(name, type, other, desc, value)
  char *name;
  unsigned char type;
  char other;
  short int desc;
  unsigned long int value;
{
  struct nlist *sym;
  sym = (struct nlist *) AppendToBlock(symBlock, (long int) sizeof(struct nlist));
  sym->n_un.n_strx = strBlock->length;
#if CANREVERSEVM
  if (header.a_magic == XL_OMAGIC &&
      (type == N_TEXT || type == N_TEXT+N_EXT))
    type += N_DATA - N_TEXT;
#endif CANREVERSEVM
  sym->n_type = type;
  sym->n_other = other;
  sym->n_desc = desc;
  sym->n_value = value;
  os_strcpy(AppendToBlock(strBlock, os_strlen(name)+1), name);
}

public procedure ObjPutTextSym(name)
  char *name;
{
  ObjPutSymbol(name, N_TEXT+N_EXT, 0, 0, header.a_text);
}

public procedure ObjPutBSSSym(name, count)
  char *name; long int count;
{
  ObjPutSymbol(name, N_BSS+N_EXT, 0, 0, header.a_bss);
  header.a_bss += count;
}

private Block *MakeBlock(maxLength)
  long int maxLength;
{
  Block *block = (Block *) os_malloc((long int) sizeof(Block));
  block->base = os_malloc(maxLength);
  block->maxLength = maxLength;
  block->length = 0;
  return block;
}

private procedure FreeBlock(block)
  Block *block;
{
  os_free(block->base);
  os_free((char *) block);
}

private char *AppendToBlock(block, size)
  Block *block; long int size;
  /* returned pointer is valid only until next call */
{
  if ((block->length += size) > block->maxLength) {
    block->maxLength = block->length + (block->length>>1);
    block->base = os_realloc(block->base, block->maxLength);
    }
  return block->base + block->length - size;
}
