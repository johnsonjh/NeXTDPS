/*
  fontrun.c

Copyright (c) 1989-1990 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Mark Francis: Fri Sep  1 12:41:24 1989
Edit History:
Larry Baer: Mon Nov 27 08:45:09 1989
Mark Francis: Fri Oct 12 09:58:28 1990
End Edit History.
*/

/*---------------------------------------------------------------------

		Overview of disk based font mechanism

Adobe  type  1 fonts can be treated two ways by DPS. If a font is
defined via the run operator, all the information needed to render a
character in that font is  resident in VM in the form of various
dictionaries, strings, and PostScript procedures. If a font is defined
via the fontrun operator, DPS treats it  as  a disk  based  font. This
means when the outline description for a character must be executed,
the string containing the description "program" is not resident in VM
but must be fetched from the original font file on disk.

The motivation for disk based fonts is the reduction of VM space
consumption at the expense  of  a  potential  disk  access to retrieve
an outline description. The format of the type 1 font files is
identical for both the VM resident and  disk based treatments. The disk
based mechanism parses the file and executes it in a selective fashion
to create the necessary data structures to support  the  disk accesses
required.

A  disk  based font contains all the "named" components of a VM
resident font plus a CharOffsets string which will be described below.
Beside the CharOffsets string,  the only difference in internal
representation between VM resident and disk based fonts is the content
of the CharStrings dictionary.  In  a  VM  font each  entry  in  the
CharStrings dict is a string containing a PostScript-like language
which, when executed by the font machinery, creates  a  path  for  the
outline of that character. In a disk based font, each CharStrings dict
entry is an integer which acts as an index into the font's CharOffsets
string.

The modified CharStrings dictionary and the CharOffsets string are
created by the actions of the fontrun operator, not by the font's
PostScript  program.  To do  this,  execution  of  the  font  file must
be carefully controlled and some actions of the font program must be
subverted or prevented altogether.

As a side effect, the disk based font mechanism helps determine where
the space for a given font will be allocated. It is desirable for 
most fonts to be allocated in shared VM. However, some type 3 fonts
incorrectly create what should be private data structures when in
shared VM allocation mode - this bug prevents their correct operation.
As a policy, fontrun assumes it is dealing with a type 1 font and
sets shared allocation mode. If it later discovers the font is not
a type 1 font, it reverts to the original allocation mode and
restarts execution of the font at the beginning. See the "Recovery"
section for more details.

Design constraints:

   - minimize  memory  use  - beyond not building all the CharStrings data
     into VM,  the  design  should  keep  to  a  minimum  the  memory
     for structures needed to access the character descriptions on
     disk.

   - compatability - the mechanism should not break existing PostScript
     programs.  All known customizations to type 1 fonts  should
     continue to work.

   - speed - the mechanism should not execute a font program significantly
     slower than executing the font via the run operator.  Actually,
     this mechanism will be faster than run for most cases.

This mechanism depends on the following features of Adobe type 1 fonts:

   - Each  character description (within  the CharStrings dictionary)
     is defined via PS code that looks like /name n <RD> "data" <ND> 
     where  n is the number of bytes of data for the character. <RD>
     is a single token of type "executable name object". It may
     consists of any sequence of characters legal in a name object.

   - All "poly fonts" (like Optima) test a variable  named hires to
     determine which set of Subrs and CharStrings definitions to use.
     The first   set  of  Subrs  and  CharStrings  definitions  apply
     to  low resolution usage and the second apply to high resolution
     usage.

   - All "synthetic fonts" (like Helvetica-Oblique) look up the base font
     in  FontDirectory  -  this  lookup  occurs sometime after the
     Private dictionary is created. The exact syntax fontrun  is
     looking  for  is "FontDictionary /Helvetica known".

   - In all compressed font formats, only the part of the file 
     containing encrypted data is compressed.

   - In the PC compressed font format, the entire compressed part of the
     file  is  described  by  a  single "segment header". Specifically,
     no character outline description string will contain a segment
     header.

   - In the Mac compressed font format, the font file contains no data
     fork.  No resource exceeds 2048 bytes in length.

---------------------------------------------------------------------*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include GC
#include LANGUAGE
#include ORPHANS
#include PSLIB
#include STREAM
#include VM

#include "fontsnames.h"
#include "fontbuild.h"
#include "fontrun.h"

/* globals */

private char charMap[] = "CharMap";
#define CHARMAP_LEN 7

private char adobeType1FontComment[] = "%!PS-AdobeFont-1.";
private char otherType1FontComment[] = "%!FontType1-"; 

/* unique ID for each CharOffsets string - incremented each time 
   new string is created. */

private Card32 stringGeneration;	

/* Inline procedures */

#define InvlReturn(state)		\
    {state->validType1Font = false;	\
    return;}

#define ValidateObj(ob, t)		\
  if (ob.type != t) {			\
    state->validType1Font = false;	\
    return;}


/*---------------------------------------------------------------------

			Compressed Font Format

Unlike  the  run  operator,  fontrun  is  prepared  to  handle  font
files in compressed binary format in addition to normal Ascii format.
Before any  actual parsing  begins,  it  looks at the first few bytes
of the file to determine its format. If it is a compressed format, an
additional stream is created to filter out  the  "segment  headers"
found in the compressed format files. This stream becomes the uppermost
input stream, drawing its data from  the  original  input stream.  It
is not necessary to decompress data from the compressed segments in the
file. The decryption code handles the compressed binary format directly
and only  the  encrypted  part of the file is in the compressed format.
Ironically, this should make processing compressed  files  slightly
faster  than  "normal" format  fonts.  Specific  info  on  which
formats  will  be handled when I get marketing feedback.

The code that handles compressed format fonts is all included within
if COMPRESSED conditionals; it is not compiled by default.

---------------------------------------------------------------------*/

/* Forward declarations */

#if COMPRESSED
private Stm CreateFilterStm();
long int GetIntBE();
#endif COMPRESSED


/* Determine font file format and set up a filter stream if necessary */

private procedure DetermineFormat(state)
  PFontrunState state;
  {
  Stm istm, fstm;
  int c;
  long int hdrOffset;
#define LINE_LEN 20
  char firstLine[LINE_LEN];
  int tokenLen;

  state->format = FmtAscii;
  hdrOffset = 0;
  istm = GetStream(state->inputStm);
  if (!istm->flags.positionable)
    InvlReturn(state);
#if COMPRESSED
  switch (c = getc(istm)) {
    case '%':	/* normal "uncompressed" format */
      break;
    case 128:	/* may be IBM compressed format */
      if ((c = getc(istm)) != 1)
	/* Next byte must be a 1 (ASCII text indicator) */
	InvlReturn(state);
      state->format = FmtIBM;
      break;
    case 0:     /* Mac compressed format */
      if ((hdrOffset = GetIntBE(istm, 3)) == EOF)
        InvlReturn(state);
      if (hdrOffset != MAC_HEADER_LENGTH)
        InvlReturn(state);
      state->format = FmtMac;
      break;
    default:	/* not any of expected chars - can't be type 1 font */
      InvlReturn(state);
    }

  /* Position to first "segment header" in the file */

  if (fseek(istm, hdrOffset, 0) == EOF)
    InvlReturn(state);

  /* If file is compressed format, set up a filter stream to remove the
     segment headers and provide continuous text to fontrun */

  if (state->format != FmtAscii) {
    boolean origShared = CurrentShared();
    Object ob, cmd;

    fstm = CreateFilterStm(state->inputStm, state->format);
    SetShared(true);
    DURING {MakePStm(fstm, Xobj, &state->inputStm);}
    HANDLER {fclose(fstm); SetShared(origShared); RERAISE;} END_HANDLER;
    SetShared(origShared);
    state->inputStm.access = xAccess;

    /* Pop the old input stream, run cmd, and input stream for
       error recovery from the Exec stack and push our own
       input stream (and original run cmd object) */

    EPopP(&ob); EPopP(&cmd); EPopP(&ob);
    EPushP(&state->inputStm); EPushP(&cmd); EPushP(&state->inputStm);
    }
#endif COMPRESSED

  /* Read the first line of the font and look for a comment that begins
     with either "%!PS-AdobeFont-1." or "%!FontType1-"; all conforming
     type 1 fonts must use one of these comments. If neither found,
     reject the font as an invalid type 1 font. */

  istm = GetStream(state->inputStm);
  (void) os_fgets(firstLine, LINE_LEN, istm);
  if (fseek(istm, hdrOffset, 0) == EOF)
    InvlReturn(state);
  tokenLen = os_strlen(adobeType1FontComment);
  firstLine[tokenLen] = '\0';
  if (os_strcmp(adobeType1FontComment, firstLine) != 0) {
    tokenLen = os_strlen(otherType1FontComment);
    firstLine[tokenLen] = '\0';
    if (os_strcmp(otherType1FontComment, firstLine) != 0)
      InvlReturn(state);
    }
  }

#if COMPRESSED
private int ComStmFilBuf(), ComStmUnGetc(), ComStmFlush(), ComStmClose(), 
  ComStmFAvail(), ComStmFSeek(), ReadIBMSegHdr(), ReadMacSegHdr();;

private long int ComStmFRead(), ComStmFTell();

private readonly StmProcs ComStmProcs = {
  ComStmFilBuf, StmErr, ComStmFRead, StmZeroLong, ComStmUnGetc, ComStmFlush,
  ComStmClose, ComStmFAvail, StmErr, StmErr, ComStmFSeek, ComStmFTell,
  "Compressed"};

typedef int (*RdHdrProc)();

/* Definitions of overlays for various fields of the standard StmRec 
   that are not otherwise used by this stream class, and extensions
   to the standard StmRec */
   
#define BytesInSeg(stm) *(long int *)&(stm)->data.a
#define ReadSegHdr(stm) *(RdHdrProc *)&(stm)->data.b
#define ComStmSource(stm) *(StmObj *)((char *)(stm)+sizeof(StmRec))

#define BAD_HDR -2

/* Get a "Big Endian" integer from the specified stream.
   Normally returns a positive integer. Returns EOF if the
   stream runs dry. */

long int GetIntBE(stm, nBytes)
  Stm stm;
  int nBytes;
  {
  long int val = 0;
  int i, c;

  for (i = nBytes - 1; i >= 0; i--)
    if ((c = getc(stm)) == EOF)
      return(EOF);
    else
      val |= c << (8 * i);
  return(val);
  }

/* Get a "Little Endian" integer from the specified stream.
   Normally returns a positive integer. Returns EOF if the
   stream runs dry. */

long int GetIntLE(stm, nBytes)
  Stm stm;
  int nBytes;
  {
  long int val = 0;
  int i, c;

  for (i = 0; i < nBytes; i++)
    if ((c = getc(stm)) == EOF)
      return(EOF);
    else
      val |= c << (8 * i);
  return(val);
  }

/* Create a filter stream for the specified file format */

private Stm CreateFilterStm(source, fmt)
  StmObj source;	/* underlying "source" stream object */
  Card16 fmt;		/* type of compressed file */
  {
  Stm fstm;

  fstm = StmCreate(&ComStmProcs, (integer)sizeof(StmObj));
  fstm->flags.read = true;
  fstm->flags.positionable = true;
  ComStmSource(fstm) = source;
  BytesInSeg(fstm) = 0;
  switch (fmt) {
    case FmtIBM: ReadSegHdr(fstm) = ReadIBMSegHdr; break;
    case FmtMac: ReadSegHdr(fstm) = ReadMacSegHdr; break;
    default: CantHappen();
    }
  return(fstm);
  }

private ReadSegHeader(stm, ustm)
  Stm stm;	/* filter stream */
  Stm ustm;	/* underlying stream */
  {
  long int pos;

  stm->cnt = 0;
  if ((pos = ftell(ustm)) == EOF)
    return(EOF);
  switch (BytesInSeg(stm) = (*ReadSegHdr(stm))(ustm)) {
    case BAD_HDR:
      /* if seg hdr is bad, reset underlying stream to its position
         before we tried to read the header, and just let the
         reader make the best of the mess that will follow (most
         likely, some sort of language error will occur). Set
         the "number of bytes till next seg header" impossibly
         high so no more header reads will be attempted. */
      (void) fseek(ustm, pos, 0);
      BytesInSeg(stm) = MAXInt32;
      break;
    case EOF:
      /* Make underlying stream appear to be at EOF */
      ustm->cnt = 0;
      ustm->flags.eof = true;
      return(EOF);
    }
  return(0);
  }

/* Read an IBM format segment header from the stream underlying
   the compressed format filter. Return either the number
   of bytes in the segment, EOF if a file error occurs, or
   BAD_HDR if the header is incorrectly formatted. */

int ReadIBMSegHdr(stm)
  register Stm stm;
  {
  register int c;
  long int segSize;

  if (getc(stm) != 128)
    return(BAD_HDR);
  if (((c = getc(stm)) < 1) || (c > 3))
    return(BAD_HDR);
  if (c == 3)
    return(EOF);
  segSize = GetIntLE(stm, 4);
  return((segSize >= 0) ? segSize : BAD_HDR);
  }

int ReadMacSegHdr(stm)
  register Stm stm;
  {
  int   type, mbz;
  long int recSize;
 
  if (((recSize = GetIntBE(stm, 4)) < 2) || (recSize > 2048))
    return(BAD_HDR);
  type = getc(stm);
  if ((mbz = getc(stm)) != 0)
    return(BAD_HDR);
 
  switch (type) {
    case 0: /* comment segment */
    case 1: /* Ascii segment */
    case 2: /* binary segment */
      break;
    case 3:
    case 5:
      return(EOF);
    default:
      return(BAD_HDR);
    }   
    
  /* record size includes the type and MBZ bytes (which we
     just read), so account for their absense */
  return(recSize - 2);
  }
 
/* Procedures to implement the standard stream operations for 
   ~compressed file format" streams. */

private int ComStmFilBuf(stm)
  register Stm stm;
  {
  register integer c;
  register Stm ssh;

  if (stm->flags.eof)
    return EOF;
  ssh = GetStream(ComStmSource(stm));
  while ((BytesInSeg(stm) -= 1) < 0)
    if (ReadSegHeader(stm, ssh) != 0)
      break;
  c = getc(ssh);
  if (c < 0) {stm->flags.eof = 1; return EOF;}
  return c;
  }

private long int ComStmFRead(buf, itemSize, nItems, stm)
  char *buf;
  long int itemSize, nItems;
  Stm stm;
  {
  register Stm ssh;
  long int requested, want, total, got;

  total = 0;
  ssh = GetStream(ComStmSource(stm));
  requested = itemSize * nItems;
  while (requested > 0) {
    want = requested;
    if (requested > BytesInSeg(stm))
      want = BytesInSeg(stm);
    got = fread(buf, 1, want, ssh);
    total += got;
    if (got < want)
      break;
    requested -= got;
    if ((BytesInSeg(stm) -= got) == 0)
      while (BytesInSeg(stm) <= 0)
        if (ReadSegHeader(stm, ssh) != 0) {
	  requested = 0;
          break;
	  }
    }
  return(total / itemSize);
  }

private int ComStmUnGetc(ch, stm)
  Stm stm;
  {
  register Stm ssh;
  /* BUF! put in own buffer */
  ssh = GetStream(ComStmSource(stm));
  ch = ungetc(ch, ssh);
  if (ch < 0) {stm->flags.eof = 1; return EOF;}
  BytesInSeg(stm) += 1;
  return ch;
  }

private int ComStmFlush(stm)
  Stm stm;
  {
  Stm ssh;
  integer res;
  ssh = GetStream(ComStmSource(stm));
  res = fflush(ssh);
  /* BUF! Flush own buffer here */
  return res;
  }

private int ComStmClose(stm)
  Stm stm;
  {
  StmObj sStmObj; Stm ssh;
  sStmObj = ComStmSource(stm);
  StmDestroy(stm);
  /* do following after StmDestroy since it can raise ioerror */
  ssh = GetStream(sStmObj);
  if (feof(ssh)) CloseFile(sStmObj, false);
  return 0;
  }

private int ComStmFAvail(stm)
  Stm stm;
  {
  Stm ssh;
  if (stm->flags.eof) return EOF;
  ssh = GetStream(ComStmSource(stm));
  return favail(ssh);
  }

/* This is a very specialized implementation of fseek for a
   filter stream. It only handles seeks to the beginning of the
   file, which is all fontrun expects it to do, and seeks initiated from
   the procedure that fetches CharString data on demand (OCFetchFromDisk). 
   OCFetchFromDisk must set up the filter stream state before calling
   fseek; this state allows the filter stream to skip over segment
   headers it may encounter in CharStrings data. A general fseek
   would be difficult to implement given that this procedure doesn't know where
   the segment headers are in the file, and so can't set up the
   filter stream state so it knows when to process a header. */

private int ComStmFSeek(stm, offset, base)
  Stm stm;
  long int offset;
  int base;
  {
  Stm ssh;
  if (offset == 0L && base == 0)
    /* When positioning to BOF, caller doesn't set stream state so
       we have to do it here */
    BytesInSeg(stm) = 0;
  ssh = GetStream(ComStmSource(stm));
  return fseek(ssh, offset, base);
  }

private long int ComStmFTell(stm)
  Stm stm;
  {
  Stm ssh;
  ssh = GetStream(ComStmSource(stm));
  return ftell(ssh);
  }
#endif COMPRESSED


/*---------------------------------------------------------------------

			Utility routines

The parser associates tokens with actions through various 
KeywordAction tables; it keeps track of parse state by pushing and
popping these tables (simple stack maintained in the state
structure) as appropriate for the current task.

---------------------------------------------------------------------*/

/* Forward declarations of keyword action procedures */

private procedure KADict(), KABegin(), KAPutDef(), KAEexec(), KAClosefile(),
	KACharStrings(), KASubrs(), KAEndCSDict(), KACharDef(), KAPutCSDict(),
	KADefinefont(), KAFontDirectory(), KAKnown();

/* Keyword tables of commands and literal names recognized during
   'normal' font parsing. */

private KeywordAction execKeywordsTable[] = {
  { nm_dict, KADict }
 ,{ nm_begin, KABegin }
 ,{ nm_def, KAPutDef }
 ,{ nm_put, KAPutDef }
 ,{ nm_eexec, KAEexec }
 ,{ nm_definefont, KADefinefont }
 ,{ nm_closefile, KAClosefile }
 ,{ nm_FontDirectory, KAFontDirectory }
 ,{ 0, NULL }
  };

private KeywordAction litKeywordsTable[] = {
  { nm_CharStrings, KACharStrings }
 ,{ nm_Subrs, KASubrs }
 ,{ 0, NULL }
  };

private procedure NewKeywordTables(state, execKWtbl, litKWtbl)
  PFontrunState state;
  PKeywordAction execKWtbl, litKWtbl;
  {
  Assert(state->prevExecKWs == NULL);
  state->prevExecKWs = state->execKeywords;
  state->prevLitKWs = state->litKeywords;
  state->execKeywords = execKWtbl;
  state->litKeywords = litKWtbl;
  }

private procedure RestoreKeywordTables(state)
  PFontrunState state;
  {
  state->execKeywords = state->prevExecKWs;
  state->litKeywords = state->prevLitKWs;
  state->prevExecKWs = NULL;
  state->prevLitKWs = NULL;
  }

private procedure Cleanup(state)
  PFontrunState	state;
  {
  if (state->prev != NULL) {
    Cleanup(state->prev);
    state->fontName = NULL;	/* only free fontName string once! */
    }
  if (state->fontName != NULL)
    os_free(state->fontName);
  if (state->charData != NULL)
    os_free(state->charData);
  os_free(state);
  }

private PFontrunState NewState(curState)
  PFontrunState curState;
  {
  PFontrunState state;

  state = (PFontrunState) os_sureCalloc((long int) sizeof(FontrunState), 1L);
  if (curState != NULL) {
    /* Copy current state into newly allocated record, make "current"
       record hold new state */
    *state = *curState;
    os_bzero(curState, (long int) sizeof(FontrunState));
    curState->inputStm = state->inputStm;
    curState->origStm = state->origStm;
    curState->fontName = state->fontName;
    curState->format = state->format;
    curState->flags = (state->flags & F_EEXEC_SEEN);
    curState->prev = state;
    state = curState;
    }
  state->exec = true;
  state->validType1Font = true;
  state->curDict = fontDict;
  NewKeywordTables(state, execKeywordsTable, litKeywordsTable);

  return(state);
  }

private procedure RestoreState(state)
  PFontrunState	state;
  {
  FontrunState oldState;
  PFontrunState pState = state->prev;

  oldState = *state;
  *state = *pState;
  oldState.prev = NULL;
  oldState.fontName = NULL;
  *pState = oldState;
  Cleanup(pState);
  }

private procedure ActOnKeyword(state, table, pobj)
  PFontrunState	state;
  PKeywordAction table;
  register PObject pobj;
  {
  register PKeywordAction kw;
  register PNameEntry pne = pobj->val.nmval;
  register int index;

  for (kw = table; kw->action != NULL; kw++) {
    index = kw->nameIndex;
    if ((index == MATCH_ANY_NAME) ||
	(fontsNames[index].val.nmval == pne)) {
      (*kw->action)(state);
      break;
      }
    }
  }


/*---------------------------------------------------------------------

			Font Validation

As the font file is being parsed and selectively executed, validation
of  its "type  1ness" is occurring. An alternate approach might be to
read a fair sized chunk of the file into a buffer and make a scanning
pass over  it  looking  for certain  keywords and value matches. This
approach was discarded mainly because it involves an extra pass over
the file and it would mean  building  2  parsers instead  of  one
(although  the  validation  parser  would be very simple). An effective
job of validation can be performed as  a  side  effect  of  the  main
parsing  at  the  cost  of a slight hitch in the recovery scheme if the
file is found to fail the validation criteria (more on this in the
Recovery section).

The validation criteria are as follows:

   - The FontType of the file must be 1.

   - The password in the Private dictionary must equal BUILTINKEY (5839).

   - If  a FontrunType key is present in the Private dictionary, its value
     must equal 1. This key is an escape which allows fontrun to
     recognize and  give  up  on  type 1 fonts which don't follow the
     current set of font syntax assumptions. This allows the font group
     to produce type 1 fonts   that   violate   current  fontrun
     assumptions  and  set  the FontrunType key to a value that fontrun
     will recognize as invalid.

Some  fonts include 2 sets of Subrs and CharStrings; one set is
optimized for low resolution devices and the second set for  higher
resolution. These nasty fonts are known as "hybrid" or "poly" fonts and
include the Optima and Eras families.  The "correct" set is chosen when
the font  is defined according to the resolution of the current device
(rather than at Character building time as  one  might  expect).  This
is  accomplished  by testing  the  device  resolution  against  a limit
defined in the font file and setting a variable called hires "true" if
the high resolution,  second  set  of Subrs and CharStrings should be
used (set false by default).

fontrun  handles poly fonts by looking for any def or put operations
that set a variable called hires. When Subrs and CharStrings
definitions are seen,  they are  either  processed  or  discarded
based  on the presence and value of this variable.

---------------------------------------------------------------------*/

private procedure KABegin(state)
  PFontrunState state;
  {
  if (state->curDict == fontDict) {
    TopP(&state->fontDict);
    state->curDict = someOtherDict;
    }
  }

/* Action procedures for keyword variables set by def or put */

private procedure KAFontType(state)
  PFontrunState state;
  {
  Object value;

  TopP(&value);
  state->flags |= F_FONTTYPE_SEEN;
  state->validType1Font =  (value.val.ival == 1);
  }

private procedure KAPassword(state)
  PFontrunState state;
  {
  Object value;

  TopP(&value);
  state->flags |= F_PASSWORD_SEEN;
  state->validType1Font =  (value.val.ival == BUILTINKEY);
  }

private procedure KAFontrunType(state)
  PFontrunState state;
  {
  Object value;

  TopP(&value);
  state->validType1Font = (value.val.ival == FONTRUN_LEVEL);
  }

private procedure KAHires(state)
  PFontrunState state;
  {
  Object value;

  state->poly = true;
  TopP(&value);
  state->useHiRes = value.val.bval;
  }

private KeywordAction defIntKeywordsTable[] = {
  { nm_FontType, KAFontType }
 ,{ nm_password, KAPassword }
 ,{ nm_FontrunType, KAFontrunType }
 ,{ 0, NULL }
  };

private KeywordAction defBoolKeywordsTable[] = {
  { nm_hires, KAHires }
 ,{ 0, NULL }
  };

private procedure KAPutDef(state)
  PFontrunState state;
  {
  Object	ob;
  Object	nameOb;
  PKeywordAction tbl;

  TopP(&ob);
  switch (ob.type) {
    case intObj:
      tbl = defIntKeywordsTable;
      break;
    case boolObj:
      tbl = defBoolKeywordsTable;
      break;
    default:
      return;
    }
  
  /* Take selective action on the keyword found */

  IPop(opStk, &ob);
  TopP(&nameOb);
  IPush(opStk, ob);
  ActOnKeyword(state, tbl, &nameOb);
  }


/*---------------------------------------------------------------------

				Decryption

When  the  eexec operator is seen, this means lots of encrypted data
follows.  When encountered in normal font execution, eexec causes the
PSEExec  procedure to  be called. This procedure basically creates an
input stream that sucks data from the original input stream and filters
it, providing clear PostScript  code and  data  to  its reader. PSEExec
also figures out what kind of encrypted data it's dealing with (either
'hex' or 'binary') and adjusts its  stream  procs  to handle that
case.

Luckily  for  fontrun,  the  implementation of PSEExec is such that it
can be called directly from the  action  routine  that  handles  the
eexec  operator.  PSEExec 'returns' the decryption stream it creates by
pushing the stream object on the exec stack. fontrun pops this object
off  the  stack  and  replaces  the original  input stream object in
the current state with this new stream object.  On subsequent calls to
StmToken, this stream will  be  the  one  passed.  As  a validity
check  before calling PSEExec, fontrun will make sure the top operand
is the same as the current input stream. If not, it will bail out  as
per  the discussion in the Validation section.

In normal execution of a font file, the end of encrypted data is
signalled by executing the sequence "currentfile closefile". Data
following that sequence is clear PostScript. Closing the input stream
simply reinstates the original input stream as the current one, and
execution of this stream continues  without  the decryption filter.

fontrun must get control before the decryption stream is closed because it
must reestablish the original input stream in the state structure. To
do so, it looks  for  a  closefile  operator.  If  the  operand  to
closefile matches the decryption stream, this is the correct point for
fontrun to switch back to  the original stream. The close operation is
then allowed to proceed.

A file stream to the font file with the decryption filter stream in
front must be available to read character outlines from disk. In order
to reconstruct the  decryption  stream  at  some  later  time, it is
only necessary to know whether the font file contains "hex-ascii" characters
or compressed format binary characters in its encrypted section. This
information is passed to MakeDecryptionStm (in the language package)
and it takes care of the stream construction details.

---------------------------------------------------------------------*/

private procedure KAEexec(state)
  PFontrunState state;
  {
  Object stm;

  TopP(&stm);
  if (stm.val.stmval != state->inputStm.val.stmval)
    InvlReturn(state);
  state->flags |= F_EEXEC_SEEN;
  PSEExec();
  ETopP(&state->inputStm);

  /* Disable execution of the eexec operator */

  state->oneShot = true;
  state->exec = false;
  }

private procedure KAClosefile(state)
  PFontrunState state;
  {
  Object stm;

  TopP(&stm);
  if (stm.val.stmval != state->inputStm.val.stmval) 
    return;

  /* Decrypt stream should be at top of exec stack. Pop it off
     and restore original input stream. */

  ETopP(&stm);
  if (stm.val.stmval != state->inputStm.val.stmval) 
    CantHappen();
  EPopP(&stm);
  state->inputStm = state->origStm;
  }


/*---------------------------------------------------------------------

			Subrs processing

The Subrs array is only of interest to fontrun when processing a
poly font (like Optima); it is ignored otherwise. A poly font has
2 sets of Subrs data, and only one set is of interest. Older versions
of poly fonts have a bug that results in the last set of Subrs
seen being the one that gets used; this can result in a mismatch 
between the Subrs and CharStrings data. To handle this case, fontrun
looks for the characteristic sequence of tokens in the old poly
fonts and conditionally executes only the correct set of Subrs
definitions. The old poly fonts wrapped the Subrs definition in
save/restore pairs, where the restore was executed if this is
the wrong set of Subrs. Once the /Subrs name is seen, if a "save"
is seen before the next "def", this means we are processing a
newer poly font, so no special action needs to occur.

Another variant of poly font Subrs definitions looks like this:
<assume private dict on operand stack>
dup hires{userdict/fsmkr save put}if
/Subrs 99 array
dup 0 15 RD -data- noaccess put
<98 more entries like above>
hires{pop pop fsmkr restore}{userdict/fsmkr save put}ifelse
/Subrs 46 array
dup 0 15 RD -data- noaccess put
<45 more entries like above>
hires not{pop pop fsmkr restore}if
noaccess put

fontrun cannot differentiate between this form and the older, bugular
form: the "save" preceeds /Subrs, and anyway, is embedded in a 
procedure body which is invisible to fontrun. So, like the oldest form,
fontrun must conditionally execute only the correct set of Subrs
definitions. To do so, it keys on the "hires" keyword
"hires{pop pop fsmkr restore}{userdict/fsmkr save put}ifelse" and 
"hires not{pop pop fsmkr restore}if" expressions. When this name
is seen, it means the end of the set of Subrs definitions to be ignored
has been seen, and normal execution can resume.

---------------------------------------------------------------------*/

private procedure KAArraySubrs(), KASaveSubrs(), KARdSubrsData(), 
		  KAHiresSubrs(), KADefSubrs();

private KeywordAction subrsKeywordsTable[] = {
  { nm_array, KAArraySubrs }
 ,{ nm_save, KASaveSubrs }
 ,{ nm_def, KADefSubrs }
 ,{ nm_hires, KAHiresSubrs }
 ,{ MATCH_ANY_NAME, KARdSubrsData }
 ,{ 0, NULL }
  };

private procedure KASubrs(state)
  PFontrunState state;
  {
  if (!state->poly)
    return;

  if ((state->flags & F_VALID_FONT) != F_VALID_FONT)
    InvlReturn(state);
  NewKeywordTables(state, subrsKeywordsTable, NULL);
  }

/* If a "save" is seen after "Subrs", this is a new style
   poly font with the bug fixed. Don't need to take any more
   special action. */

private procedure KASaveSubrs(state)
  PFontrunState state;
  {
  RestoreKeywordTables(state);
  }

private procedure KAArraySubrs(state)
  PFontrunState state;
  {
  boolean skip = false;

  if (state->flags & F_SUBRS_SEEN) {
    /* Already used low res Subrs - skip this set */
    skip = !state->useHiRes;
  } else {
    /* If supposed to use hi res (second) set of Subrs and this is
       the first set, skip over it. */
    state->flags |= F_SUBRS_SEEN;
    skip = state->useHiRes;
    }
  if (skip) {
    /* Skip over this set - pop the array size and name from the 
       stack and stop interpretation */
    IPopDiscard(opStk); IPopDiscard(opStk);
    state->exec = false;
    }
  else 
    /* Execute this set - don't need to meddle until next /Subrs seen */
    RestoreKeywordTables(state);
  }

private procedure KARdSubrsData(state)
  PFontrunState state;
  /* The syntax of a Subrs definition in the old style poly fonts
     is:
	dup <index> <nbytes> <RD> <n binary bytes> noaccess put
     and according to the type 1 spec is:
	dup <index> <nbytes> <RD> <n binary bytes> <NP>

     Whenever the parser encounters an executable name processing
     Subrs definitions, this procedure is called. It may be called
     for the <RD> token in which case it must skip over the data
     for this Subr. It is also called for tokens like "dup", "put",
     "noaccess", and <NP>, in which case it should do nothing. 
     
     The two cases are distinguished by the type of the object at 
     the top of the operand stack.  If integer, its the <RD> case 
     (type is dictionary for other name tokens).
  */
  {
#define BUF_SIZE 128
  char buf[BUF_SIZE];
  Object dataLen;
  Stm stm = GetStream(state->inputStm);
  int count, remaining;

  /* Skip over the data if this is <RD> case, return otherwise */

  TopP(&dataLen);
  if (dataLen.type != intObj) 
    return;
  IPop(opStk, &dataLen);	/* get <nbytes> */
  IPopDiscard(opStk);		/* pop <index> */
  remaining = dataLen.val.ival;
  for (count = os_min(BUF_SIZE, remaining); 
       remaining > 0; 
       remaining -= count, count = os_min(BUF_SIZE, remaining)) 
    if (fread(buf, sizeof(char), count, stm) != count)
      InvlFont();
  }

private procedure KADefSubrs(state)
  PFontrunState state;
  {
  /* At end of "skipped" set of Subrs - turn interpreter back on after
     skipping over the "def" operator. */
  state->oneShot = true;
  RestoreKeywordTables(state);
  }

private procedure KAHiresSubrs(state)
  PFontrunState state;
  {
  /* At end of "skipped" set of Subrs - turn interpreter back on so
     hires value is pushed and normal execution resumes. Push two
     garbage objects which will be popped by the upcoming "if" or
     "ifelse" operation */
  state->exec = true;
  RestoreKeywordTables(state);
  IPush(opStk, iLNullObj); IPush(opStk, iLNullObj);
  }


/*---------------------------------------------------------------------

			Handling CharStrings Definition

When the CharStrings keyword  is  recognized,  a  flag  is  set  to
indicate CharStrings  definitions will follow soon. When the next dict
operator is seen, it checks this flag. The parameter to dict that
defines the dictionary size  is popped  from  the  op  stack  and
used  to  allocate a structure to accumulate CharStrings info. This
structure contains an entry for each  character  in  the font's
character  set. An entry consists of a name object, the data length for
the charstring data, the  decrypt  key  corresponding  to  the  first
byte  of charstring  data,  the offset in the font file for the first
byte of charstring data, and an index field.

An 'inhibit execution' flag is set in  the  state  structure;  this
prevents fontrun's   main   scanning   loop   from  calling
psExecute.  Until  the  end corresponding to this dict operator is
seen, no PostScript from the  font  file is  executed but operands will
continue to be pushed onto the op stack. The set of keywords and
action  routines  normally  used  in  fontrun's  main  loop  is
switched to a set which is specific to CharStrings processing.

Individual  charstrings definitions are now processed: the definitions
are of the form "/name n RD "data" ND".  When  the  RD  token  is
seen,  the  current decryption  key  and  font  file position (obtained
via ftell) are saved in the current charstring info entry.  The op
stack  now  looks  like  "'data  length' /name";  the  length and the
name object are popped from the op stack and saved in the current
charstring info entry (the name reference is safe  from  garbage
collection  due to our friend the "private garbage collector stack").
We don't need the data at this point so it is simply skipped over (we
know the length).

When the end operator is seen, the CharStrings definitions are
complete.  The next  put  operator  marks  the  point at which the
CharStrings key is normally entered into the base font dictionary. The
op stack  contains  the  CharStrings name  object and the font
dictionary object.  fontrun now attempts to make some sense of the
information it has accumulated.  Since  one  goal  is  to  use  as
little  VM  as  possible,  fontrun  attempts  to  share an existing
CharStrings dictionary if it can find  one  that  suits  its  purpose.
"Cromfonts"  use  a CharStrings  dictionary  (called  CharMap, in the
internalDict) that is exactly what fontrun needs if this font uses the
isoadobe standard  character  set.  To make  this  determination,  the
character name in each entry of the charstrings info structure is
looked up in this CharMap dictionary. When  a  match  occurs, the
character index from the CharMap entry is saved in the current
charstrings info entry.  If all our entries match, this font uses a
proper  subset  of  the standard character set and fontrun can make
this font's CharStrings entry point to the CharMap dictionary. If a
mismatch occurs, a unique  dictionary  must  be created  for  the font.
If necessary the dictionary is created via direct calls to the
procedures that implement the def and dict operators rather than
trying to feed the equivalent PostScript code to psExecute through a
temporary stream.  It is actually simpler to do it this way since all
the  operands  required  are already  in the proper "scanned and
objectized" form.  The dictionary object is pushed on the op stack;
when execution  of  the  input  stream  is  allowed  to proceed, the
next put operation will create the CharStrings entry.

To  create  the  CharOffsets  string,  a  string object of the proper
size is allocated. Again, the charstrings info structure is enumerated;
an entry in the string  is  made at the "index" specified by the info
entry. The information in this string entry is the decryption key, font
file offset, and data length  for the  current  character.  

A "font info" structure is allocated at the base of the CharOffsets
string. It contains the following information:
  - a string generation counter (discussed in the outline cache 
    section)
  - the byte  offset into the string for the first byte of the font's
    file name (copied from the original operand to fontrun).
The font pathname is needed to build a filtered 
stream through which character outline descriptions can be read - 
more on this subject in the  "Character  Building"  section.   The
CharStrings dictionary entry is not created at this point; the object
is simply saved in the state structure for  later  definition
(required for proper fontrun'ing of "synthetic fonts" - more on this
elsewhere).

Finally, the "inhibit execution" flag in the state structure is reset
and the initial set of keywords and action routines are restored so
fontrun's main loop can continue as before.

---------------------------------------------------------------------*/

private procedure KACharStrings(state)
  PFontrunState state;
  {
  if ((state->flags & F_VALID_FONT) != F_VALID_FONT)
    InvlReturn(state);
  state->curDict = charStrDict;
  }

private procedure KACharDef(state)
  PFontrunState state;
  /* Whenever the parser encounters an executable name processing
     CharStrings definitions, this procedure is called. It may be called
     for the <RD> token in which case it must skip over the data
     for this CharString entry. It may also be called for the <ND> 
     token, in which case it should do nothing. The two cases are 
     distinguished by the type of the object at the top of the 
     operand stack.  If integer, its the <RD> case (type is 
     dictionary for <ND> case).
  */
  {
#define BUF_SIZE 128
  char buf[BUF_SIZE];
  Object dataLen;
  Stm stm = GetStream(state->inputStm);
  long int count, remaining;

  /* Skip over the data if this is <RD> case, return otherwise */

  TopP(&dataLen);
  if (dataLen.type != intObj) 
    return;
  IPop(opStk, &dataLen);
  remaining = dataLen.val.ival;
  if (state->skipThisSet) {
    IPopDiscard(opStk);
  } else {
    PCharData cd = &(state->charData[state->curChar++]);

    cd->len = dataLen.val.ival;
    IPop(opStk, &cd->charName);
    cd->fileOffset = ftell(stm);
    cd->key = GetStmDecryptionKey(stm);
#if COMPRESSED
    count = MAXCard16;	/* "Don't care" value for bytesTillNextHdr */
    if (state->format != FmtAscii) {
      Stm fstm = GetStream(state->origStm);
      if (BytesInSeg(fstm) < remaining) {
	/* If next segment header appears in char data,
	   remember its location, else set the don't care value */
	count = BytesInSeg(fstm);
	state->flags |= F_SEG_HDR_IN_DATA;
	}
      } 
    cd->bytesTillNextHdr = count;
#endif COMPRESSED
    }

  /* Skip over the char data */

  for (count = os_min(BUF_SIZE, remaining); 
       remaining > 0; 
       remaining -= count, count = os_min(BUF_SIZE, remaining)) 
    if (fread(buf, sizeof(char), count, stm) != count)
      InvlFont();
  }

private procedure KAEndCSDict(state)
  PFontrunState state;
  {
  StrObj coStr;
  integer strLen;
  DictObj csDict;
  NameObj csName, cmName;
  IntObj charIndexObj;
  PKeyVal pKeyVal;
  Stm stm = GetStream(state->inputStm);
  register PCharData cd;
  register PCharOffsetsEntry ce;
  PFontInfo fi;
  register int i;
  boolean stdCharSet;
  Card16 maxCharIndex = 0;
  int notdefIndex = -1;
  Card16 charsInFont;

  /* Switch the keyword tables back to original state, skip
     execution of the 'end' operator but cause execution to
     begin again on next token */

  RestoreKeywordTables(state);
  state->oneShot = true;
  
  /* If we're ignoring this set of data, have to leave something 
     on the stack for the font program to pop (it expects a 
     CharStrings dict). Then bail out. */

  if (state->skipThisSet) {
    IPush(opStk, iLNullObj);
    return;
    }

  /* We assume the CharStrings name object is at top of the 
     operand stack and the base font dictionary is just below it. */

  TopP(&csName);
  ValidateObj(csName, nameObj);

  /* state->curChar == actual number of characters to put in the CharStrings
     dict. state->maxNumChar is the size of the CharStrings dict to
     create; this value may be greater than the actual number of
     characters in the font, so use state->curChar as the actual value. */

  charsInFont = state->curChar;

  /* Try to use an existing CharMap dictionary if all characters in
     this font's char set are in isoadobe std char set. A proper
     subset of isoadobe is OK - in this case, must remember the
     maximum character index (to size the CharOffsets string) and
     the index of the .notdef character (must fill in all "holes"
     in the CharOffsets string to point to the .notdef character
     outline description). */

  if (stdCharSet = (state->maxNumChars <= ISOADOBE_CHARSET_SIZE)) {
    FastName(charMap, CHARMAP_LEN, &cmName);
    Begin(rootShared->vm.Shared.internalDict);
    stdCharSet = Load(cmName, &csDict);
    End();
    }
  for (i = 0; stdCharSet && (i < charsInFont); i++) {
    cd = state->charData + i;
    if (stdCharSet = SearchDict(csDict.val.dictval, cd->charName, &pKeyVal)) {
      cd->index = pKeyVal->value.val.ival;
      if (state->maxNumChars < ISOADOBE_CHARSET_SIZE) {
        maxCharIndex = os_max(maxCharIndex, cd->index);
	if ((notdefIndex < 0) && 
	    (os_strcmp(".notdef", cd->charName.val.nmval->str) == 0))
	  notdefIndex = cd->index;
	}
      }
    }

  if (stdCharSet) {
    if (state->maxNumChars < ISOADOBE_CHARSET_SIZE) 
      state->isoadobeSubset = true;
    else
      maxCharIndex = charsInFont - 1;
  } else {
    /* If can't share the CharMap dict, must create our own CharStrings
       dict. */ 
    maxCharIndex = charsInFont - 1;
    DictP((cardinal) state->maxNumChars, &csDict);
    for (i = 0, cd = state->charData; i < charsInFont; i++, cd++) {
      cd->index = i;
      LIntObj(charIndexObj, cd->index);
      ForcePut(csDict, cd->charName, charIndexObj);
      }
    }

  /* Push the completed CharStrings dictionary object on the stack.
     It will be def'ed by the font program */

  IPush(opStk, csDict);

  /* Allocate a string object for the CharOffsets string, fill in all
     the entries, and copy the font pathname in at the end. The string
     must be large enough to hold entries for all characters in the
     set, plus the FontInfo structure at the base of the string, 
     plus the null-terminated font pathname. 

     The storage for the string MUST be aligned on the preferred
     alignment boundary to avoid problems on RISC machines or
     architectures that require aligned short or long word accesses. */

#define NUM_ENTRIES (maxCharIndex + 1)	/* 0..maxCharIndex */
  strLen = sizeof(FontInfo) + 
	   (NUM_ENTRIES * sizeof(CharOffsetsEntry)) + 
           os_strlen(state->fontName) + 1;
  LStrObj(coStr, strLen, (charptr) AllocAligned(strLen));
  ConditionalInvalidateRecycler (&coStr);
  
  coStr.access = rAccess;
  if (state->isoadobeSubset)
    /* Make all CharOffsets string entries initially contain the data
       for the .notdef char. When we fill in the actual char data,
       the "holes" in the charset will still look like .notdef */
    for (i = 1, cd = state->charData + notdefIndex, 
	 ce = CharOffsetsPtr(coStr.val.strval);
	 i <= NUM_ENTRIES;
	 i++, ce++) {
      ce->offset = cd->fileOffset;
      ce->len = cd->len;
      ce->key = cd->key;
      }
  for (i = 0; i < charsInFont; i++) {
    cd = state->charData + i;
    ce = CharOffsetsPtr(coStr.val.strval) + cd->index;
    ce->offset = cd->fileOffset;
    ce->len = cd->len;
    ce->key = cd->key;
#if COMPRESSED
    ce->bytesTillNextHdr = cd->bytesTillNextHdr;
#endif COMPRESSED
    }
  fi = FontInfoPtr(coStr.val.strval);
  fi->generation = stringGeneration++;
  fi->fileNameOffset = sizeof(FontInfo) + 
		       (NUM_ENTRIES * sizeof(CharOffsetsEntry));
  (void) os_strcpy((char *)(coStr.val.strval + fi->fileNameOffset), 
    state->fontName);
#if COMPRESSED
  fi->fontFmt = state->format;
#endif COMPRESSED

  /* Enter the CharOffsets string in the base font dictionary. If
     we're operating on the base font for a synthetic font, copy the
     string object to the synthetic font's state for later use. */

  DictPut(state->fontDict, fontsNames[nm_CharOffsets], coStr);
  if (state->prev != NULL)
    state->prev->charOffsetsStr = coStr;
  }

/* Keywords recognized during CharStrings dict parsing */

private KeywordAction charStringsKeywordsTable[] = {
  { nm_end, KAEndCSDict }
 ,{ MATCH_ANY_NAME, KACharDef }
 ,{ 0, NULL }
  };

private procedure KADict(state)
  PFontrunState state;
  {
  Object sizeDict;

  if (state->synthetic && (state->curDict != fontDict)) 
    /* Once a font is recognized as synthetic, encountering a
       dict operator means the base font is being created. Create a
       new state block for the base font */
    state = NewState(state);

  if (state->curDict == fontDict) {
    /* Create the base font dictionary. Increment the max entries
       to allow later addition of CharOffsets entry */
    IPop(opStk, &sizeDict);
    if (sizeDict.type != intObj)
      TypeCheck();
    sizeDict.val.ival++;
    IPush(opStk, sizeDict);
  } else if (state->curDict == charStrDict) {
    /* About to create the CharStrings dictionary. Prevent
       this dict operator from executing and start special
       CharStrings parsing */
    if (state->flags & F_CHARSTRINGS_SEEN) {
      /* If already used low res CharStrings defs, skip this set */
      state->skipThisSet = !state->useHiRes;
    } else {
      /* If supposed to use hi res (second) set of CharStrings defs and 
         this is the first set, skip over it. */
      state->flags |= F_CHARSTRINGS_SEEN;
      state->skipThisSet = state->useHiRes;
      }

    state->exec = false;
    NewKeywordTables(state, charStringsKeywordsTable, NULL);

    /* Pop size of dict (size of character set). If using this set
       of CharStrings defs, allocate a char data array large 
       enough to accumulate info for each character def */

    IPop(opStk, &sizeDict);
    if (!state->skipThisSet) {
      state->maxNumChars = sizeDict.val.ival;
      state->charData = 
        (PCharData) os_sureMalloc(state->maxNumChars * sizeof(CharData));
      }
    }
  }


/*---------------------------------------------------------------------

			Synthetic fonts

Synthetic  fonts  are  adaptions  of  some  base  font,  where  the
base font's characteristics are modified in some manner (i.e., by
changing the font  matrix for  obliqueness or changing the character
widths for a narrow font). Synthetic fonts try to share the CharStrings
of the base font; they do so by testing  for the base font's existence
and then creating their CharStrings entry to point to the base font's
CharStrings dictionary.  If the base  font  has  not  yet  been
defined,  the  synthetic  font defines it; it contains a complete
definition of the base font within itself and this definition is
executed  if  necessary.  If the base font already exists, this
definition is skipped over.

In  order  to  correctly  handle  these  ugly  beasts,  fontrun  must
add  a CharOffsets entry to the synthetic font's dictionary. It tries
to recognize the test  a  synthetic  uses to find its base font. If a
font file does a lookup in the FontDirectory after creation of the
Private dictionary  has  begun,  it  is assumed  to  be a synthetic
font.  fontrun catches the upcoming known operator, makes its own
test  for  the  existence  of  the  base  font,  and  saves  its
CharOffsets object in the state structure if the font exists.

If  the base font does not exist, fontrun recognizes its creation by
noticing a new "base font dictionary" being built after it has
already  determined  the font  is  synthetic.  In  this  case,  a  new
instance of the global "state" is instantiated (old state is pushed),
and the base font definition  occurs  as  a normal  font.  When  the
definefont  operation  for this base font is about to occur, its
CharOffsets object will be copied  to  the  synthetic  font's  state
record.

When the definefont operator is seen at the end of synthetic font
creation, a CharOffsets entry (using the saved CharOffsets object) is
added  to  its  font dictionary before the definefont is executed.

---------------------------------------------------------------------*/

private KeywordAction synthKeywordsTable[] = {
  { nm_known, KAKnown }
 ,{ 0, NULL }
  };

private procedure KAFontDirectory(state)
  PFontrunState state;
  {
  if (state->curDict != fontDict) {
    /* FontDirectory seen after font dictionary created means
       this is a synthetic font. Note this fact and set up to
       capture the next "known" operation */
    state->synthetic = true;
    NewKeywordTables(state, synthKeywordsTable, NULL);
    }
  }

/* Assumes stack has font name and "font directory" dict on it */

private procedure KAKnown(state)
  PFontrunState state;
  {
  NameObj fname;
  DictObj fdir;

  /* Get the font name and directory dict from the op stack, lookup
     the font name in it and save font dictionary value if it
     exists */

  IPop(opStk, &fname);
  ValidateObj(fname, nameObj);
  TopP(&fdir);
  ValidateObj(fdir, dictObj);
  IPush(opStk, fname);
  if (Known(fdir, fname)) 
    DictGetP(fdir, fname, &state->baseFontDict);
  RestoreKeywordTables(state);
  }

private procedure KADefinefont(state)
  PFontrunState state;
  {
  if (state->prev != NULL)
    /* finished base font definition - switch back to synthetic font */
    RestoreState(state);
  else if (state->synthetic) {
    /* Must create the CharOffsets entry for synthetic font. If we had
       to create the base font, the CharOffsets string object was
       stuffed into our state at that time. If not there, we must
       look up the key in the base font dictionary. If base font
       doesn't have a CharOffsets string, it must have been created
       via "run" (not fontrun) - in this case, don't bother creating
       a CharOffsets string for the synthetic font. It will not
       be treated as a disk-based font. */
    if (state->charOffsetsStr.type == nullObj) {
      if (Known(state->baseFontDict, fontsNames[nm_CharOffsets]))
        DictGetP(state->baseFontDict, fontsNames[nm_CharOffsets], 
                 &state->charOffsetsStr);
      }
    if (state->charOffsetsStr.type != nullObj) 
      DictPut(state->fontDict, fontsNames[nm_CharOffsets], state->charOffsetsStr);
    }
  }


/*---------------------------------------------------------------------

			fontrun main procedure

fontrun  calls  PSRun  to  create a file stream to the filename on top
of the operand stack. When called from psExecute, run pushes this
stream on  the  exec stack   and   returns   (effectively   passing
the  stream  to  psExecute  for interpretation).  However, fontrun
remains in control of  stream  execution  at this  point.    It  enters
a loop where StmToken is called in the mode where it returns the next
object in the stream rather than pushing it on any stack.  The basic
idea is the object is compared to important keywords for its object
type, and a specific action procedure is  called  if  a  match
occurs.  The  current fontrun  "state" is passed to all action
procedures in a data structure of type FontrunState (global variables
cannot be used due to the reentrancy  constraint presented
earlier).    The action routines are free to modify elements of this
structure as necessary.

For a given token (object), fontrun switches on the object tag:

   - executable - for objects of type name, get the  object's definition.
     For  commands  and  names  whose  definitions resolve to an object
     of command type, look for the following key operators:

        * def, put - the operators to these commands are compared to a
	  set of  interesting  keywords  including  hires, 
	  FontType, password. Details of specific cases follow.

        * dict - needed to catch the creation of the base dictionary 
	  for the font.

	* eexec,closefile - much of the data in the font file following
	  the eexec operator is encrypted.  Recognition  of  this
	  keyword allows   fontrun   to   perform   some  special
	  sleight-of-hand (description follows). Recognition of
	  closefile means the end of the encrypted data.

     Array and packed array objects are pushed on the operand stack.

   - For  all  other  object types and for names and commands whose action
     routines don't prohibit it, call psExecute,  passing  the  object
     to execute.

   - literal - if the type is "name", a keyword match occurs. In all
     cases, the object is then pushed on the operand stack. The
     important names  are  CharStrings  and  Subrs.  They must be
     recognized at this point rather than catching them when they are
     "def"ed or "put"ed;  by then,  all  the  work  that  may  need
     to  be  prevented has already occurred.

				Recovery

fontrun must  be  able  to  recover  from  PostScript  errors  in  the
font, environmental  errors  like  running  out  of  memory,  and
validation errors.  Recovery actions are completely different for these
three cases.

Since the main parsing loop is calling psExecute directly, it can tell
when a PostScript error has occurred (by the return value from
psExecute). If an error is detected, the error code is obtained via a
call to GetAbort and an exception is RAISEd. This leaves cleanup in the
hands of the ultimate exception handler.

Vulnerable  parts  of  fontrun's  execution  are  enclosed  in a
standard DPS exception handler. Any exceptions caught by this handler
are passed on  to  the caller via a RERAISE.

Recovery  from validation failure is not actually "error recovery"; it
simply means the font doesn't meet our criteria as  a  type  1  font.
This  is  quite understandable,  since  all  non-VM  resident  findfont
requests will end up in fontrun, including type 3 fonts. Recovery  in
this  case  consists  of  having fontrun take itself out of the picture
but arrange for execution of the font to continue as if via the run
operator. The input stream position will be reset to its beginning. 
All objects pushed on the operand stack by execution of the font will 
be popped off - their storage will be freed by a subsequent shared VM 
garbage collection or by the recycler (if we ever teach it how to handle 
dicts).

In all cases, the VM allocation mode will be reset to its value at the
beginning of fontrun execution.

---------------------------------------------------------------------*/

public procedure PSFontRun()
  {
  PFontrunState	state = NULL;
  StrObj	fontNameStr;
  Object	ob;
  boolean	origShared;
  cardinal	origOpStk, curOpStk;
  cardinal	origDictStk, curDictStk;

  state = NewState(state);

  /* Copy the font name for later use - top of operand stack should
     contain the fontname string */

  TopP(&fontNameStr);
  if (fontNameStr.type != strObj) 
    TypeCheck();

  state->fontName = 
    (charptr) os_sureCalloc((long int) fontNameStr.length + 1, 1L);
  (void) os_strncpy(state->fontName, fontNameStr.val.strval,
    fontNameStr.length);

  /* Call PSRun to set up the initial file stream. It pushes the 
     stream it creates on the exec stack. Determine format of file
     (ascii, compressed), and set up a filter stream if necessary. */

  PSRun();
  ETopP(&state->inputStm);
  DetermineFormat(state);

  /* Remember the current input stream as the 'original' stream
     for later use, and remember the state of the operand and
     dictionary stacks for recovery if necessary. */

  state->origStm = state->inputStm;
  origOpStk = CountStack(opStk, MAXcardinal);
  origDictStk = CountStack(dictStk, MAXcardinal);
  origShared = CurrentShared();

  DURING
    SetShared(true);
    while (state->validType1Font) {
      if (!StmToken(GetStream(state->inputStm), &ob, false)) 
	/* EOF on stream */
	break;
      switch (ob.tag) {
	case Lobj:	/* Literal object */
          if ((state->litKeywords != NULL)  && (ob.type == nameObj))
            ActOnKeyword(state, state->litKeywords, &ob);
          IPush(opStk, ob);
	  break;
	case Xobj:	/* Executable object */
	  switch (ob.type) {
	    case nameObj:
	    case cmdObj:
	      ActOnKeyword(state, state->execKeywords, &ob);
	      break;
	    default:	/* Just push on the operand stack */
	      IPush(opStk, ob);
	      continue;
	    }
	  if (state->exec) {
	    if (psExecute(ob))
	      /* Error encountered - raise an exception */
	      RAISE((int) GetAbort(), (charptr) NULL);
	  } else if (state->oneShot) {
            state->oneShot = false;
            state->exec = true;
            }
	  break;
	} /* Switch on tag */
	/* Don't put any code here - it will break the above 'continue' */
      }
  HANDLER
    SetShared(origShared);
    Cleanup(state);
    RERAISE;
  END_HANDLER
  
  if (! state->validType1Font) {
    /* If type 1 validity check failed, 
       position the input stream to its beginning. Pop the operand,
       dict stacks back to original state to remove references to any
       objects created so far. The input stream object is still on 
       top of the exec stack, so simply returning to our caller at
       this point will permit execution of the stream as if by
       PSRun. */
    for (curOpStk = CountStack(opStk, MAXcardinal);
         curOpStk > origOpStk; curOpStk--)
      IPopDiscard(opStk);
    for (curDictStk = CountStack(dictStk, MAXcardinal);
         curDictStk > origDictStk; curDictStk--)
      IPopDiscard(dictStk);
    (void) fseek(GetStream(state->inputStm), 0L, 0);
    }

  /* Reset allocation mode to original state, release memory and
     return. */

  SetShared(origShared);
  Cleanup(state);

}

 
/*---------------------------------------------------------------------

			The Open Font File Cache

We keep streams open to the most recently accessed font files  to
avoid  the overhead  of  a file system lookup prior to each character
data read operation.  This is accomplished by maintaining an open file
cache; each entry identifies a stream  object  open  to  a  unique
font  file.  The lookup key is a structure containing the font's
CharOffsets string address and its generation count.  The
GetFontStream  procedure  is  called  with  such  a key and it returns
a stream if the cache contains a matching entry or, in the case
of  a  miss,  the miss resolution procedure is able to create a new
stream to the requested file.

In  order  to  resolve  a  cache  miss,  the  original font file name
must be obtained from the CharOffsets string (passed as part of the
lookup  key).    If unable  to  open this file, the miss resolution
attempt fails and GetFontStream will return an error indication to its
caller.

Cache invalidation is handled in the same manner as for  the  outline
cache; invalid  entries  are  allowed to fall off the LRU chain and are
protected from incorrect hits by the generation counter.

---------------------------------------------------------------------*/

/* Get a stream to a font file */

private PCache fileCache;


private PFileEntry GetFontStream(fontInfo)
  PFontInfo fontInfo;	/* ptr to info at head of CharOffsets string */
  {
  PFileEntry pEntry;
  FileTag fTag;

  fTag.pStr = (string) fontInfo;
  fTag.generation = fontInfo->generation;
  if (!CacheLookup(fileCache, (CaTag) &fTag, (CaData) &pEntry))
    RAISE(ecIOError, (char *)NULL);
  return(pEntry);
  }

/* File cache policy procedures - see the discussion in
   cache.h for relevant semantics and assumptions */

private boolean FCOpenStream(pFTag, data, size, pHandle)
  PFileTag *pFTag;
  CaData *data;
  int *size;
  PCacheEntHdr *pHandle;
  {
  PFileEntry pEntry;
  char *fontFile;
  register PFontInfo fi;
  StmObj src;
  DecryptionType stmKind;

  /* If one or more entries will be displaced to accomodate the new 
     one, try to reuse one.  If not, allocate a new entry, set up
     a fake stream object and body */

  *size = 1;
  if ((pEntry = (PFileEntry) CacheReuseEntry(fileCache, size, pHandle))
      == NULL)
    {
    pEntry = (PFileEntry) os_sureMalloc(sizeof(FileEntry));
    LStmObj(pEntry->source, 0xFADE, &pEntry->body);
    pEntry->body.generation = 0xFADE;
#if COMPRESSED
    LStmObj(pEntry->filter, 0xFADE, &pEntry->filterBody);
    pEntry->filterBody.generation = 0xFADE;
#endif COMPRESSED
    }
  pEntry->tag = **pFTag;
  *pFTag = &pEntry->tag;
  fi = FontInfoPtr(pEntry->tag.pStr);
  fontFile = (char *) fi + fi->fileNameOffset;
  pEntry->body.stm = CreateFileStm(fontFile, "r");
  src = pEntry->source;
  stmKind = hexStream;
#if COMPRESSED
  if (fi->fontFmt == FmtAscii) {
    pEntry->filterBody.stm = NULL;
  } else {
    /* Font file is compressed format with segment headers
       within char data. Must construct a segment header filter
       stream on top of the source stream. */
    pEntry->filterBody.stm = 
      CreateFilterStm(pEntry->source, fi->fontFmt);
    src = pEntry->filter;
    stmKind = binStream;
    }
#endif COMPRESSED
  pEntry->dstm = MakeDecryptionStm(src, stmKind);
  *data = (CaData) pEntry;
  return(true);
  }


private int FCHashId(tag)
  PFileTag tag;
  {
  return(FileHashId(tag));
  }

private boolean FCMatch(tag1, tag2)
  PFileTag tag1, tag2;
  {
  return((tag1->pStr == tag2->pStr) &&
	 (tag1->generation == tag2->generation));
  }

private procedure FCFreeEntry(pEntry, dispose)
  PFileEntry pEntry;
  boolean dispose;
  {
  fclose(pEntry->dstm);
#if COMPRESSED
  if (pEntry->filterBody.stm != NULL)
    fclose(pEntry->filterBody.stm);
#endif COMPRESSED
  fclose(pEntry->body.stm);
  if (dispose)
    os_free((char *) pEntry);
  }


/*---------------------------------------------------------------------

			Building Characters

The  procedure  that  controls  building  characters  for  type  1  fonts  is
InternalBuildChar. Prior to disk based type 1 fonts, InternalBuildChar
operated by looking up the specified character name in the font's
CharStrings dictionary and switching on the object type of the value
for that name key. String objects were  passed  directly  to CCRun for
execution and array objects were passed to PSE to be interpreted as
PostScript programs.

Since the object type of the entries  in  a  disk  based  font's
CharStrings dictionary is integer, InternalBuildChar now includes a
case statement for type intObj.  This case calls FetchCharOutline (in
fontrun.c)  to  return  a  string containing  a character outline
description, and passes this string to CCRun to create the path for the
outline.

The outline cache exists as an optimization to avoid having to go to
disk  to fetch  a  character  description.  Each  entry  in  the cache
contains the data representing the outline description for a single
character in some base  font.  The number of entries in the cache is
controlled by comparing the total size of the data for each entry to a
maximum limit (to be  determined  experimentally).  All  entries  are
maintained  in  order  of reference; the least recently used entry(ies)
is displaced if addition of a new outline  exceeds  the  cache  size
limit.  The  cache  size limit is not under control of a PostScript
program but can be set at compile time.

Entries in this cache are tagged  by  a  data  structure  consisting
of  the pointer to a font's CharOffsets string, a generation count, and
the character's index into this string.  The pointer to  a  particular
CharOffsets  string  is known  only  to  fonts  that  share  the  same
character outline data. Thus, a reencoded font derived from a disk
based font will share the CharOffsets string with  its parent font, and
will benefit by sharing entries in the outline cache with its parent
font (and vice-versa).

If a matching entry is found,  it  is  moved  to  the  "most  recently
used" position  in  the  cache's LRU chain, and the outline data is
made available to the requestor. If a miss occurs, the outline data
must be fetched from disk.

Caches are your friend until you  are  faced  with  how  to
invalidate  them correctly  and  efficiently.  An entry in the outline
cache is invalid whenever the corresponding CharOffsets string is
garbage collected or disappears due  to a  restore  or  VM
destruction.  However  this mechanism does not try to purge invalid
entries when any of these events occur.  Doing  so  is  more  expensive
than  is  warranted  in  this  case.  Invalid entries are allowed to
age and be displaced from the cache. Once the corresponding
CharOffsets  string  has  been destroyed,  the  invalid  entries
should  not  be referenced and so should age quickly in terms of
continuing cache accesses. However,  Murphy's  Law  implies that
sooner  or  later, another CharOffsets string will be created at the
same address as a previous CharOffsets string. That's why the cache
tag  contains  a "generation  count" component; each time a new
CharOffsets string is created by fontrun, a global generation counter
is incremented and its value is stashed in entry  0  of  the  string.
The  counter  is  a  32  bit number; this should be sufficient to
prevent incorrect outline cache hits.

---------------------------------------------------------------------*/

/* Fetch a character's outline from the cache or from disk */

private PCache outlineCache;

public procedure FetchCharOutline(offsetStr, charIndex, pobj)
  StrObj offsetStr;	/* font's CharOffsets string */
  int charIndex;	/* index of character in CharOffsets string */
  PObject pobj;		/* ptr to result string object */
  {
  POutlineEntry pEntry;
  OutlineTag oTag;
  register PFontInfo fi;

  oTag.pStr = offsetStr.val.strval;
  fi = FontInfoPtr(oTag.pStr);
  oTag.generation = fi->generation;
  oTag.index = charIndex;

  if (!CacheLookup(outlineCache, (CaTag) &oTag, (CaData) &pEntry))
    RAISE(ecIOError, (char *)NULL);
  
  /* Don`t create a real string object from VM - just point to
     the character outline data in the cache entry. */

  LStrObj(*pobj, pEntry->len, (string) pEntry->data);
  }

/* Outline cache policy procedures - see the discussion in
   cache.h for relevant semantics and assumptions */

private boolean OCFetchFromDisk(pOTag, data, size, pHandle)
  POutlineTag *pOTag;
  CaData *data;
  int *size;
  PCacheEntHdr *pHandle;
  {
  register PCharOffsetsEntry ce;
  PFontInfo info;
  register Stm stm;
  POutlineEntry pEntry;
  PFileEntry font;

  info = FontInfoPtr((*pOTag)->pStr);
  font = GetFontStream(info);
  stm = font->dstm;

  /* Point to the character entry and position the stream */

  ce = CharOffsetsPtr(info) + (*pOTag)->index;
  if (((int)ce - (int)info) >= info->fileNameOffset)
    InvlFont();
  if (fseek(stm, ce->offset, 0) == EOF)
    RAISE(ecIOError, (char *)NULL);
  SetStmDecryptionKey(stm, (longcardinal) ce->key);

  /* If one or more entries will be displaced to accomodate the new 
     one, try to reuse one.  If not, allocate a new entry */

  *size = ce->len;
  if ((pEntry = 
      (POutlineEntry) CacheReuseEntry(outlineCache, size, pHandle)) == NULL)
    {
    long int size = sizeof(OutlineEntry) - 1 + ce->len;
    pEntry = (POutlineEntry) os_sureMalloc(size);
    }
  pEntry->tag = **pOTag;
  *pOTag = &pEntry->tag;
  pEntry->len = ce->len;
#if COMPRESSED
  if (info->fontFmt != FmtAscii)
    /* If using a filter stream, set up the filter stream's 
       "remaining bytes till I have to eat a header" field */
    BytesInSeg(font->filterBody.stm) = ce->bytesTillNextHdr;
#endif COMPRESSED
  if (fread(pEntry->data, 1L, (long int) ce->len, stm) != ce->len)
    RAISE(ecIOError, (char *)NULL);

  *data = (CaData) pEntry;
  return(true);
  }

private int OCHashId(tag)
  POutlineTag tag;
  {
  return(OutlineHashId(tag));
  }

private boolean OCMatch(tag1, tag2)
  POutlineTag tag1, tag2;
  {
  return((tag1->index == tag2->index) && 
	 (tag1->pStr == tag2->pStr) &&
	 (tag1->generation == tag2->generation));
  }

private procedure OCFreeEntry(pEntry, dispose)
  POutlineEntry pEntry;
  boolean dispose;
  {
  if (dispose)
    os_free((char *) pEntry);
  }


/*---------------------------------------------------------------------

			Initialization

Initialization of the disk based font mechanism consists of 
registering the fontrun operator explicitly and creating the
caches for outline data and open file streams. In DEVELOP versions,
debugging operators are also registered.

---------------------------------------------------------------------*/

private CacheProcs fileCacheProcs = {
  FCHashId, FCMatch, FCOpenStream, FCFreeEntry, NULL
  };

private CacheProcs outlineCacheProcs = {
  OCHashId, OCMatch, OCFetchFromDisk, OCFreeEntry, NULL
  };

extern PVoidProc fetchCharOutline;

public procedure FontRunInit(reason)
  InitReason reason;
  {
  switch (reason) {
    case init:
      fetchCharOutline = FetchCharOutline;
      fileCache = 
        CacheCreate("fontfile", &fileCacheProcs, 
	  FILE_CACHE_LIMIT, FILE_HASH_BUCKETS);
      outlineCache = 
        CacheCreate("outline", &outlineCacheProcs, 
	  OUTLINE_CACHE_LIMIT, OUTLINE_HASH_BUCKETS);
      break;
    case romreg:
#if (STAGE==DEVELOP)
      if (vSTAGE == DEVELOP) {
        RgstExplicit("cstats", PSCacheStatistics);
        RgstExplicit("cflush", PSCacheFlush);
        RgstExplicit("cinit", PSCacheInit);
        }
#endif
      break;
    }
  }
