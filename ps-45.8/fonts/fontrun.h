/*
  fontrun.h

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

Original version: Mark Francis: Fri Sep  1 13:01:40 1989
Edit History:
Mark Francis: Tue Oct  2 11:01:11 1990
Jack Newlin: 2Oct90 maintain that "COMPRESSED 1" (bug 9794)
End Edit History.
*/

#ifndef	FONTRUN_H
#define	FONTRUN_H

#include BASICTYPES
#include ENVIRONMENT
#include "cache.h"

/* By default, disable handling of compressed format fonts */

#ifndef COMPRESSED
#define COMPRESSED 1
#endif

/* Data Types */

/* The current fontrun implementation can handle type 1 files
   whose /FontrunType keys have value <= FONTRUN_LEVEL */

#define FONTRUN_LEVEL 1

/* Number of characters in isoadobe standard character set */

#define ISOADOBE_CHARSET_SIZE 229

/* Structure used to accumulate character data during CharStrings dict
   processing */

typedef struct _t_CharData {
  Object	charName;
  Card16	index;
  Card16	len;
  cardinal	key;
  long int	fileOffset;
#if COMPRESSED
  Card16	bytesTillNextHdr;
#endif COMPRESSED
  } CharData, *PCharData;

/* Structure of each entry in the CharOffsets string */

typedef struct _t_CharOffsetsEntry {
  long int	offset;		/* offset in file of outline data */
  Card16	len;		/* # bytes of outline data */
  cardinal	key;		/* decryption key */
#if COMPRESSED
  Card16	bytesTillNextHdr;
#endif COMPRESSED
  } CharOffsetsEntry, *PCharOffsetsEntry;

/* "Font Info" struct at base of  CharOffsets string */

typedef struct _t_FontInfo {
  Card32	generation;	/* serial # of charoffsets string */
  Card16	fileNameOffset;	/* string offset of font file name */
#if COMPRESSED
  Card16	fontFmt;	/* font file format code */
#endif COMPRESSED
  } FontInfo, *PFontInfo;

/* Create pointer to first CharOffsetsEntry struct in CharOffsets 
   string.  Resulting pointer can be used in pointer arithmetic using
   the character index from a CharStrings entry. */

#define CharOffsetsPtr(p) \
  ((PCharOffsetsEntry) ((char *)(p) + sizeof(FontInfo)))

/* Coerce pointer to FontInfo structure from CharOffsets 
   string.val.strval */

#define FontInfoPtr(p) ((PFontInfo) (p))

/* Structure used to build arrays of keywords and associated action 
   procs. */

typedef struct _t_KeywordAction {
  Int16		nameIndex;		/* index in fontsNames table */
  PVoidProc	action;			/* procedure to call on match */
  } KeywordAction, *PKeywordAction;

/* A keyword action entry with the MATCH_ANY_NAME index
   value matches any name object */

#define MATCH_ANY_NAME 32767

/* Font file format codes */
#define FmtAscii	0
#define FmtMac		1
#define FmtIBM 		2

/* Structure used to pass fontrun's "global" state to support routines */

typedef enum { fontDict, someOtherDict, charStrDict } FontDict;

typedef struct _t_FontrunState {
  struct _t_FontrunState *prev;	/* ptr to "pushed" state (synthetic fonts) */
  charptr	fontName;	/* font pathname */
  DictObj	fontDict;	/* font dictionary */
  DictObj	baseFontDict;	/* base font dictionary of synthetic font */
  StrObj	charOffsetsStr;	/* CharOffsets string object */
  StmObj	inputStm;	/* current input stream */
  StmObj	origStm;	/* original input stream */
  Card16	format;		/* ascii, compressed IBM, etc */
  int		flags;		/* defined below */
  PKeywordAction execKeywords;	/* table of keyword, action for Xobj's */
  PKeywordAction litKeywords;	/* table of keyword, action for Lobj's */
  PKeywordAction prevExecKWs;	/* pushed table of Xobj keywords */
  PKeywordAction prevLitKWs;	/* pushed table of Lobj keywords */
  FontDict	curDict;	/* which dictionary is being processed */
  PCharData	charData;	/* ptr to info from CharStrings processing */
  Card16	maxNumChars;	/* # characters in font's char set */
  Card16	curChar;	/* index of current char data entry */
  BitField	exec;		/* execute each executable token parsed */
  BitField	oneShot;	/* disable execution for only one token */
  BitField	validType1Font;	/* true until validation fails */
  BitField	synthetic;	/* font shares CharStrings with base font */
  BitField	poly;		/* font has 2 sets of CharStrings, Subrs */
  BitField	useHiRes;	/* use second set of Subrs, CharStrings defs */
  BitField	skipThisSet;	/* determines which set of CharStrings defs
				   to use for Poly-Fonts (Optima, etc) */
  BitField	isoadobeSubset;	/* true if font is subset of std char set */
  } FontrunState, *PFontrunState;

/* Flags for FontrunState struct */

#define F_FONTTYPE_SEEN		0x1
#define F_EEXEC_SEEN		0x2
#define F_PASSWORD_SEEN		0x4
#define F_VALID_FONT	(F_FONTTYPE_SEEN | F_EEXEC_SEEN |\
			 F_PASSWORD_SEEN)
#define F_SUBRS_SEEN		0x10
#define F_CHARSTRINGS_SEEN	0x20
#define F_SEG_HDR_IN_DATA	0x40

/* Compressed font format definitions */

#define MAC_HEADER_LENGTH 256


/* Outline cache definitions */

#define OUTLINE_CACHE_LIMIT 65536
#define OUTLINE_HASH_BUCKETS 127

#define OutlineHashId(tag) (tag->index)

typedef struct _t_OutlineTag {
  string	pStr;		/* ptr to font's CharOffsets string data */
  Card32	generation;	/* generation count for string */
  Card16	index;		/* char's index into CharOffsets string */
  } OutlineTag, *POutlineTag;

/* Outline cache entry. */ 

typedef struct _t_OutlineEntry {
  OutlineTag	tag;		/* cache tag for entry */
  Card16	len;		/* length of data currently held */
  char		data[1];	/* character outline data */
  } OutlineEntry, *POutlineEntry;


/* Font file cache definitions */

#define FILE_CACHE_LIMIT 3
#define FILE_HASH_BUCKETS (FILE_CACHE_LIMIT * 2) + 1

#define FileHashId(tag) (tag->generation)

/* Tag for entries in open font file cache */

typedef struct _t_FileTag {
  string	pStr;		/* ptr to font's CharOffsets string data */
  Card32	generation;	/* generation count for string */
  } FileTag, *PFileTag;

typedef struct _t_FileEntry {
  FileTag	tag;		/* cache tag for entry */
  Stm		dstm;		/* decryption stream */
  StmObj	source;		/* underlying file stream object */
  StmBody	body;		/* fake stream body */
#if COMPRESSED
  StmObj	filter;		/* compressed format filter stream */
  StmBody	filterBody;	/* filter stream body */
#endif COMPRESSED
  } FileEntry, *PFileEntry;

/* Exported Procedures */

/* Semi-Public Procedures (used within package only) */

/* Inline Procedure Implementations */

/* Exported Data */

#endif	FONTRUN_H
