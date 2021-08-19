/*
  fontsnames.h

Copyright (c) 1988-1990 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Thu Jul  7 14:34:27 1988
Edit History:
Scott Byer: Tue Aug 21 11:17:45 1990
Ed Taft: Mon Feb  5 08:51:27 1990
Jim Sandman: Mon Apr 30 09:47:57 1990
Joe Pasqua: Wed Dec 14 15:15:00 1988
Bill Paxton: Tue Nov 14 14:29:18 1989
End Edit History.

Definition of operators and registered names for the fonts package.
This is used both as a C header file and as the input to a shell
script, update_registered_names, that derives a PostScript program
to register the names and one or more C programs to register the
C procedures for the operators.
*/

#ifndef	FONTSNAMES_H
#define	FONTSNAMES_H

/*
  The following macro defines the package index for this package.
  It must be consistent with the one defined in orphans.h.
 */

#define PACKAGE_INDEX 3  /* = pni_fonts */

/*
  The following macro calls define all standard PostScript operator names
  and their corresponding C procedures. The operators are (or can be)
  divided into multiple operator sets, each corresponding to a distinct
  language extension.

  The first two arguments of the OpSet macro specify a name and a number
  to identify the operator set. The name and number must both be unique
  within the package (the number is regenerated automatically by
  update_registered_names). The third argument identifies the dictionary
  in which the operators are to be registered.
 */

OpSet(ops_fonts1, 1, systemdict)
  Op(ashow, PSAShow)
  Op(awidthshow, PSAWidthShow)
  Op(cachestatus, PSCacheStatus)
  Op(currentcacheparams, PSCrCParams)
  Op(currentfont, PSCrFont)
  Op(definefont,PSDefineFont)
  Op(internaldict, PSIntDict)
  Op(kshow, PSKShow)
  Op(makefont, PSMakeFont)
  Op(scalefont, PSScaleFont)
  Op(setcachedevice, PSSetCchDevice)
  Op(setcachelimit, PSSetCacheLimit)
  Op(setcacheparams, PSStCParams)
  Op(setcharwidth, PSSetCharWidth)
  Op(setfont, PSSetFont)
  Op(show, PSShow)
  Op(stringwidth, PSStrWidth)
  Op(widthshow, PSWidthShow)

OpSet(ops_fonts2, 2, systemdict)
  Op(selectfont, PSSelectFont)
  Op(undefinefont,PSUnDefineFont)
  Op(xshow, PSXShow)
  Op(xyshow, PSXYShow)
  Op(yshow, PSYShow)

/* Continuation of ops_fonts2 -- composite font operators that were an
   extension to level 1 are now treated strictly as level 2 features */
  Op(cshow, PSCShow)
  Op(rootfont, PSRootFont)
  Op(setcachedevice2, PSSetCchDevice2)

/* No additional ops for DPS */

OpSet(ops_fontsInternal, 3, internaldict)
  Op(CCRun, PSCCRun)
  Op(eCCRun, PSeCCRun)
  Op(ErodeSW, PSErodeSW)
  Op(fontrun, PSFontRun)
  Op(lck, PSLck)
  Op(strtlck, StartLock)
  Op(xlck, PSSetXLock)
  Op(ylck, PSSetYLock)

/*
  The following macros define the indices for all registered names
  other than operators. The actual PostScript name is simply
  the macro name with the "nm_" prefix removed.

  The macro definitions are renumbered automatically by the
  update_registered_names script. For ease of maintenance, they
  should be in alphabetical order.
 */

#define nm_BitmapWidths 0
#define nm_BlueFuzz 1
#define nm_BlueScale 2
#define nm_BlueShift 3
#define nm_BlueValues 4
#define nm_BuildChar 5
#define nm_CDevProc 6
#define nm_CharData 7
#define nm_CharData1 8
#define nm_CharOffsets 9
#define nm_CharStrings 10
#define nm_CurMID 11
#define nm_Encoding 12
#define nm_Erode 13
#define nm_EscChar 14
#define nm_ExactSize 15
#define nm_ExpansionFactor 16
#define nm_FDepVector 17
#define nm_FID 18
#define nm_FMapType 19
#define nm_FontBBox 20
#define nm_FontDirectory 21
#define nm_FontInfo 22
#define nm_FontMatrix 23
#define nm_FontName 24
#define nm_FontrunType 25
#define nm_FontType 26
#define nm_FudgeBands 27
#define nm_KernVector 28
#define nm_InBetweenSize 29
#define nm_ISOLatin1Encoding 30
#define nm_Metrics 31
#define nm_Metrics2 32
#define nm_MIDVector 33
#define nm_MinFeature 34
#define nm_OrigFont 35
#define nm_OtherBlues 36
#define nm_OtherSubrs 37
#define nm_PaintType 38
#define nm_PrefEnc 39
#define nm_Private 40
#define nm_RndStemUp 41
#define nm_RunInt 42
#define nm_ScaleMatrix 43
#define nm_StandardEncoding 44
#define nm_StdHW 45
#define nm_StdVW 46
#define nm_StrokeWidth 47
#define nm_Subrs 48
#define nm_SubsVector 49
#define nm_TCFont 50
#define nm_TransformedChar 51
#define nm_UniqueID 52
#define nm_WMode 53
#define nm_aover 54
#define nm_array 55
#define nm_ascend 56
#define nm_baseline 57
#define nm_begin 58
#define nm_bover 59
#define nm_capheight 60
#define nm_capover 61
#define nm_closefile 62
#define nm_def 63
#define nm_definefont 64
#define nm_descend 65
#define nm_dict 66
#define nm_eexec 67
#define nm_end 68
#define nm_engineclass 69
#define nm_erosion 70
#define nm_findfont 71
#define nm_halfsw 72
#define nm_hires 73
#define nm_known 74
#define nm_lenIV 75
#define nm_locktype 76
#define nm_notdef 77 /* .notdef */
#define nm_overshoot 78
#define nm_password 79
#define nm_put 80
#define nm_save 81
#define nm_strokewidth 82
#define nm_xheight 83
#define nm_xover 84
#define nm_StemSnapH 85
#define nm_StemSnapV 86
#define nm_FamilyBlues 87
#define nm_FamilyOtherBlues 88
#define nm_WeightVector 89
#define nm_rndwidth 90
#define nm_idealwidth 91
#define nm_gsfactor 92

/* The following definition is regenerated by update_registered_names */
#define NUM_PACKAGE_NAMES 93

extern PNameObj fontsNames;

#endif	FONTSNAMES_H
