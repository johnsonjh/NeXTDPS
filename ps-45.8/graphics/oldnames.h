/*
  graphicsnames.h

Copyright (c) 1988 Adobe Systems Incorporated.
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
Ed Taft: Fri Jul  7 16:04:58 1989
Ivor Durham: Thu Aug 18 09:20:29 1988
Paul Rovner: Fri Aug 18 15:35:58 1989
End Edit History.

Definition of operators and registered names for the graphics package.
This is used both as a C header file and as the input to a shell
script, update_registered_names, that derives a PostScript program
to register the names and one or more C programs to register the
C procedures for the operators.
*/

#ifndef	GRAPHICSNAMES_H
#define	GRAPHICSNAMES_H

/*
  The following macro defines the package index for this package.
  It must be consistent with the one defined in orphans.h.
 */

#define PACKAGE_INDEX 2  /* = pni_graphics */

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

OpSet(ops_graphicsDPS, 1, systemdict)
  Op(arc, PSArc)
  Op(arcn, PSArcN)
  Op(arct, PSArcT)
  Op(arcto, PSArcTo)
  Op(charpath, PSCharPath)
  Op(clip, PSClip)
  Op(clippath, PSClipPath)
  Op(closepath, PSClosePath)
  Op(colorimage, PSColorImage)
  Op(concat, PSCnct)
  Op(concatmatrix, PSCnctMtx)
  Op(copypage, PSCopyPage)
  Op(currentblackgeneration, PSCrBlkGeneration)
  Op(currentcmykcolor, PSCrCMYKColor)
#if NOTDPS
  Op(currentcolor, PSCrColor)
  Op(currentcolorspace, PSCrColorSpace)
#endif NOTDPS
  Op(currentcolorscreen, PSCrColorScreen)
  Op(currentcolortransfer, PSClrTransfer)
  Op(currentdash, PSCrDash)
  Op(currentflat, PSCrFlatThreshold)
  Op(currentgray, PSCrGray)
  Op(currentgstate, PSCurrentGState)
  Op(currenthalftone, PSCrHalftone)
  Op(currenthalftonephase, PSCurrentHalftonePhase)
  Op(currenthsbcolor, PSCrHSBColor)
  Op(currentlinecap, PSCrLineCap)
  Op(currentlinejoin, PSCrLineJoin)
  Op(currentlinewidth, PSCrLineWidth)
  Op(currentmatrix, PSCrMtx)
  Op(currentmiterlimit, PSCrMiterLimit)
  Op(currentpoint, PSCrPoint)
  Op(currentrgbcolor, PSCrRGBColor)
  Op(currentscreen, PSCrScreen)
  Op(currentstrokeadjust, PSCurrentStrokeAdjust)
  Op(currenttransfer, PSCrTransfer)
  Op(currentundercolorremoval, PSCrUCRemoval)
  Op(curveto, PSCurveTo)
  Op(defaultmatrix, PSDfMtx)
  Op(deviceinfo, PSDeviceInfo)
  Op(dtransform, PSDTfm)
  Op(eoclip, PSEOClip)
  Op(eofill, PSEOFill)
  Op(eoviewclip, PSEOViewClip)
  Op(erasepage, PSErasePage)
  Op(fill, PSFill)
  Op(flattenpath, PSFltnPth)
  Op(grestore, GRstr)
  Op(grestoreall, GRstrAll)
  Op(gsave, GSave)
  Op(gstate, PSGState)
  Op(identmatrix, PSIdentMtx)
  Op(idtransform, PSIDTfm)
  Op(image, PSImage)
  Op(imagemask, PSImageMask)
  Op(ineofill, PSInEOFill)
  Op(infill, PSInFill)
  Op(initclip, InitClip)
  Op(initgraphics, InitGraphics)
  Op(initmatrix, InitMtx)
  Op(initviewclip, PSInitViewClip)
  Op(instroke, PSInStroke)
  Op(inueofill, PSInUEOFill)
  Op(inufill, PSInUFill)
  Op(inustroke, PSInUStroke)
  Op(invertmatrix, PSInvertMtx)
  Op(itransform, PSITfm)
  Op(lineto, PSLineTo)
  Op(matrix, PSMtx)
  Op(moveto, PSMoveTo)
  Op(newpath, NewPath)
  Op(nulldevice, NullDevice)
  Op(pathbbox, PSPathBBox)
  Op(pathforall, PSPathForAll)
  Op(rcurveto, PSRCurveTo)
  Op(rectclip, PSRectClip)
  Op(rectfill, PSRectFill)
  Op(rectstroke, PSRectStroke)
  Op(rectviewclip, PSRectViewClip)
  Op(reversepath, PSReversePath)
  Op(rlineto, PSRLineTo)
  Op(rmoveto, PSRMoveTo)
  Op(rotate, PSRtat)
  Op(scale, PSScal)
  Op(setbbox, PSSetBBox)
  Op(setblackgeneration, PSSetBlkGeneration)
#if NOTDPS
  Op(setcmykcolor, PSSetCMYKColor)
  Op(setcolor, PSSetColor)
#endif NOTDPS
  Op(setcolorscreen, PSSetColorScreen)
#if NOTDPS
  Op(setcolorspace, PSSetColorSpace)
#endif NOTDPS
  Op(setcolortransfer, PSSetClrTransfer)
  Op(setdash, PSSetDash)
  Op(setflat, PSSetFlatThreshold)
  Op(setgray, PSSetGray)
  Op(setgstate, PSSetGState)
  Op(sethalftone, PSSetHalftone)
  Op(sethalftonephase, PSSetHalftonePhase)
  Op(sethsbcolor, PSSetHSBColor)
  Op(setlinecap, PSSetLineCap)
  Op(setlinejoin, PSSetLineJoin)
  Op(setlinewidth, PSSetLineWidth)
  Op(setmatrix, PSSetMtx)
  Op(setmiterlimit, PSSetMiterLimit)
  Op(setrgbcolor, PSSetRGBColor)
  Op(setscreen, PSSetScreen)
  Op(setstrokeadjust, PSSetStrokeAdjust)
  Op(settransfer, PSSetTransfer)
  Op(setucacheparams, PSSetUCacheParams)
  Op(setundercolorremoval, PSSetUCRemoval)
  Op(showpage, PSShowPage)
  Op(stroke, PSStroke)
  Op(strokepath, PSStrkPth)
  Op(transform, PSTfm)
  Op(translate, PSTlat)
  Op(uappend, PSUAppend)
  Op(ucache, NoOp)
  Op(ucachestatus, PSUCacheStatus)
  Op(ueofill, PSUEOFill)
  Op(ufill, PSUFill)
  Op(upath, PSUPath)
  Op(ustroke, PSUStroke)
  Op(ustrokepath, PSUStrokePath)
  Op(viewclip, PSViewClip)
  Op(viewclippath, PSViewClipPath)
  Op(wtranslation, PSWTranslation)

/*
  The following macros define the indices for all registered names
  other than operators. The actual PostScript name is simply
  the macro name with the "nm_" prefix removed.

  The macro definitions are renumbered automatically by the
  update_registered_names script. For ease of maintenance, they
  should be in alphabetical order.
 */

#define nm_BitsPerSample 0
#define nm_BlueValues 1
#define nm_CIELAB 2
#define nm_CIELightness 3
#define nm_CircleFont 4
#define nm_ColorValues 5
#define nm_Colors 6
#define nm_DataSource 7
#define nm_Decode 8
#define nm_DeviceCMYK 9
#define nm_DeviceGray 10
#define nm_DeviceRGB 11
#define nm_DeviceSeparation 12
#define nm_GrayValues 13
#define nm_GreenValues 14
#define nm_HalftoneType 15
#define nm_ImageMatrix 16
#define nm_ImageType 17
#define nm_Indexed 18
#define nm_InvalidFont 19
#define nm_MultipleDataSources 20
#define nm_RedValues 21
#define nm_arc 22
#define nm_arcn 23
#define nm_arct 24
#define nm_array 25
#define nm_closepath 26
#define nm_curveto 27
#define nm_cvx 28
#define nm_lineto 29
#define nm_moveto 30
#define nm_rcurveto 31
#define nm_rlineto 32
#define nm_rmoveto 33
#define nm_setbbox 34
#define nm_ucache 35

/* following must be groups of 3 in order Angle, Frequency, SpotFunction */
#define nm_Angle 36
#define nm_Frequency 37
#define nm_SpotFunction 38
#define nm_RedAngle 39
#define nm_RedFrequency 40
#define nm_RedSpotFunction 41
#define nm_GreenAngle 42
#define nm_GreenFrequency 43
#define nm_GreenSpotFunction 44
#define nm_BlueAngle 45
#define nm_BlueFrequency 46
#define nm_BlueSpotFunction 47
#define nm_GrayAngle 48
#define nm_GrayFrequency 49
#define nm_GraySpotFunction 50

/* Following must be groups of 3 in order Width, Height, Thresholds */
#define nm_Width 51
#define nm_Height 52
#define nm_Thresholds 53
#define nm_RedWidth 54
#define nm_RedHeight 55
#define nm_RedThresholds 56
#define nm_GreenWidth 57
#define nm_GreenHeight 58
#define nm_GreenThresholds 59
#define nm_BlueWidth 60
#define nm_BlueHeight 61
#define nm_BlueThresholds 62
#define nm_GrayWidth 63
#define nm_GrayHeight 64
#define nm_GrayThresholds 65

/* The following definition is regenerated by update_registered_names */
#define NUM_PACKAGE_NAMES 66

extern PNameObj graphicsNames;

#endif	GRAPHICSNAMES_H
