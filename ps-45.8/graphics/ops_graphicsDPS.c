/*
  ops_graphicsDPS.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_graphicsDPS". It was generated automatically,
using information derived from the graphicsnames.h file.
*/

{
  extern void PSArc();
  extern void PSArcN();
  extern void PSArcT();
  extern void PSArcTo();
  extern void PSCharPath();
  extern void PSClip();
  extern void PSClipPath();
  extern void PSClosePath();
  extern void PSColorImage();
  extern void PSCnct();
  extern void PSCnctMtx();
  extern void PSCopyPage();
  extern void PSCrBlkGeneration();
  extern void PSCrCMYKColor();
  extern void PSCrColorScreen();
  extern void PSClrTransfer();
  extern void PSCrDash();
  extern void PSCrFlatThreshold();
  extern void PSCrGray();
  extern void PSCurrentGState();
  extern void PSCrHalftone();
  extern void PSCurrentHalftonePhase();
  extern void PSCrHSBColor();
  extern void PSCrLineCap();
  extern void PSCrLineJoin();
  extern void PSCrLineWidth();
  extern void PSCrMtx();
  extern void PSCrMiterLimit();
  extern void PSCrPoint();
  extern void PSCrRGBColor();
  extern void PSCrScreen();
  extern void PSCurrentStrokeAdjust();
  extern void PSCrTransfer();
  extern void PSCrUCRemoval();
  extern void PSCurveTo();
  extern void PSDfMtx();
  extern void PSDeviceInfo();
  extern void PSDTfm();
  extern void PSEOClip();
  extern void PSEOFill();
  extern void PSEOViewClip();
  extern void PSErasePage();
  extern void PSFill();
  extern void PSFltnPth();
  extern void GRstr();
  extern void GRstrAll();
  extern void GSave();
  extern void PSGState();
  extern void PSIdentMtx();
  extern void PSIDTfm();
  extern void PSImage();
  extern void PSImageMask();
  extern void PSInEOFill();
  extern void PSInFill();
  extern void InitClip();
  extern void InitGraphics();
  extern void InitMtx();
  extern void PSInitViewClip();
  extern void PSInStroke();
  extern void PSInUEOFill();
  extern void PSInUFill();
  extern void PSInUStroke();
  extern void PSInvertMtx();
  extern void PSITfm();
  extern void PSLineTo();
  extern void PSMtx();
  extern void PSMoveTo();
  extern void NewPath();
  extern void NullDevice();
  extern void PSPathBBox();
  extern void PSPathForAll();
  extern void PSRCurveTo();
  extern void PSRectClip();
  extern void PSRectFill();
  extern void PSRectStroke();
  extern void PSRectViewClip();
  extern void PSReversePath();
  extern void PSRLineTo();
  extern void PSRMoveTo();
  extern void PSRtat();
  extern void PSScal();
  extern void PSSetBBox();
  extern void PSSetBlkGeneration();
  extern void PSSetCMYKColor();
  extern void PSSetColorScreen();
  extern void PSSetClrTransfer();
  extern void PSSetDash();
  extern void PSSetFlatThreshold();
  extern void PSSetGray();
  extern void PSSetGState();
  extern void PSSetHalftone();
  extern void PSSetHalftonePhase();
  extern void PSSetHSBColor();
  extern void PSSetLineCap();
  extern void PSSetLineJoin();
  extern void PSSetLineWidth();
  extern void PSSetMtx();
  extern void PSSetMiterLimit();
  extern void PSSetRGBColor();
  extern void PSSetScreen();
  extern void PSSetStrokeAdjust();
  extern void PSSetTransfer();
  extern void PSSetUCacheParams();
  extern void PSSetUCRemoval();
  extern void PSShowPage();
  extern void PSStroke();
  extern void PSStrkPth();
  extern void PSTfm();
  extern void PSTlat();
  extern void PSUAppend();
  extern void NoOp();
  extern void PSUCacheStatus();
  extern void PSUEOFill();
  extern void PSUFill();
  extern void PSUPath();
  extern void PSUStroke();
  extern void PSUStrokePath();
  extern void PSViewClip();
  extern void PSViewClipPath();
  extern void PSWTranslation();
#if (OS == os_mpw)
  void (*ops_graphicsDPS[120])();
  register void (**p)() = ops_graphicsDPS;
  *p++ = PSArc;
  *p++ = PSArcN;
  *p++ = PSArcT;
  *p++ = PSArcTo;
  *p++ = PSCharPath;
  *p++ = PSClip;
  *p++ = PSClipPath;
  *p++ = PSClosePath;
  *p++ = PSColorImage;
  *p++ = PSCnct;
  *p++ = PSCnctMtx;
  *p++ = PSCopyPage;
  *p++ = PSCrBlkGeneration;
  *p++ = PSCrCMYKColor;
  *p++ = PSCrColorScreen;
  *p++ = PSClrTransfer;
  *p++ = PSCrDash;
  *p++ = PSCrFlatThreshold;
  *p++ = PSCrGray;
  *p++ = PSCurrentGState;
  *p++ = PSCrHalftone;
  *p++ = PSCurrentHalftonePhase;
  *p++ = PSCrHSBColor;
  *p++ = PSCrLineCap;
  *p++ = PSCrLineJoin;
  *p++ = PSCrLineWidth;
  *p++ = PSCrMtx;
  *p++ = PSCrMiterLimit;
  *p++ = PSCrPoint;
  *p++ = PSCrRGBColor;
  *p++ = PSCrScreen;
  *p++ = PSCurrentStrokeAdjust;
  *p++ = PSCrTransfer;
  *p++ = PSCrUCRemoval;
  *p++ = PSCurveTo;
  *p++ = PSDfMtx;
  *p++ = PSDeviceInfo;
  *p++ = PSDTfm;
  *p++ = PSEOClip;
  *p++ = PSEOFill;
  *p++ = PSEOViewClip;
  *p++ = PSErasePage;
  *p++ = PSFill;
  *p++ = PSFltnPth;
  *p++ = GRstr;
  *p++ = GRstrAll;
  *p++ = GSave;
  *p++ = PSGState;
  *p++ = PSIdentMtx;
  *p++ = PSIDTfm;
  *p++ = PSImage;
  *p++ = PSImageMask;
  *p++ = PSInEOFill;
  *p++ = PSInFill;
  *p++ = InitClip;
  *p++ = InitGraphics;
  *p++ = InitMtx;
  *p++ = PSInitViewClip;
  *p++ = PSInStroke;
  *p++ = PSInUEOFill;
  *p++ = PSInUFill;
  *p++ = PSInUStroke;
  *p++ = PSInvertMtx;
  *p++ = PSITfm;
  *p++ = PSLineTo;
  *p++ = PSMtx;
  *p++ = PSMoveTo;
  *p++ = NewPath;
  *p++ = NullDevice;
  *p++ = PSPathBBox;
  *p++ = PSPathForAll;
  *p++ = PSRCurveTo;
  *p++ = PSRectClip;
  *p++ = PSRectFill;
  *p++ = PSRectStroke;
  *p++ = PSRectViewClip;
  *p++ = PSReversePath;
  *p++ = PSRLineTo;
  *p++ = PSRMoveTo;
  *p++ = PSRtat;
  *p++ = PSScal;
  *p++ = PSSetBBox;
  *p++ = PSSetBlkGeneration;
  *p++ = PSSetCMYKColor;
  *p++ = PSSetColorScreen;
  *p++ = PSSetClrTransfer;
  *p++ = PSSetDash;
  *p++ = PSSetFlatThreshold;
  *p++ = PSSetGray;
  *p++ = PSSetGState;
  *p++ = PSSetHalftone;
  *p++ = PSSetHalftonePhase;
  *p++ = PSSetHSBColor;
  *p++ = PSSetLineCap;
  *p++ = PSSetLineJoin;
  *p++ = PSSetLineWidth;
  *p++ = PSSetMtx;
  *p++ = PSSetMiterLimit;
  *p++ = PSSetRGBColor;
  *p++ = PSSetScreen;
  *p++ = PSSetStrokeAdjust;
  *p++ = PSSetTransfer;
  *p++ = PSSetUCacheParams;
  *p++ = PSSetUCRemoval;
  *p++ = PSShowPage;
  *p++ = PSStroke;
  *p++ = PSStrkPth;
  *p++ = PSTfm;
  *p++ = PSTlat;
  *p++ = PSUAppend;
  *p++ = NoOp;
  *p++ = PSUCacheStatus;
  *p++ = PSUEOFill;
  *p++ = PSUFill;
  *p++ = PSUPath;
  *p++ = PSUStroke;
  *p++ = PSUStrokePath;
  *p++ = PSViewClip;
  *p++ = PSViewClipPath;
  *p++ = PSWTranslation;
#else (OS == os_mpw)
  static void (*ops_graphicsDPS[120])() = {
    PSArc,
    PSArcN,
    PSArcT,
    PSArcTo,
    PSCharPath,
    PSClip,
    PSClipPath,
    PSClosePath,
    PSColorImage,
    PSCnct,
    PSCnctMtx,
    PSCopyPage,
    PSCrBlkGeneration,
    PSCrCMYKColor,
    PSCrColorScreen,
    PSClrTransfer,
    PSCrDash,
    PSCrFlatThreshold,
    PSCrGray,
    PSCurrentGState,
    PSCrHalftone,
    PSCurrentHalftonePhase,
    PSCrHSBColor,
    PSCrLineCap,
    PSCrLineJoin,
    PSCrLineWidth,
    PSCrMtx,
    PSCrMiterLimit,
    PSCrPoint,
    PSCrRGBColor,
    PSCrScreen,
    PSCurrentStrokeAdjust,
    PSCrTransfer,
    PSCrUCRemoval,
    PSCurveTo,
    PSDfMtx,
    PSDeviceInfo,
    PSDTfm,
    PSEOClip,
    PSEOFill,
    PSEOViewClip,
    PSErasePage,
    PSFill,
    PSFltnPth,
    GRstr,
    GRstrAll,
    GSave,
    PSGState,
    PSIdentMtx,
    PSIDTfm,
    PSImage,
    PSImageMask,
    PSInEOFill,
    PSInFill,
    InitClip,
    InitGraphics,
    InitMtx,
    PSInitViewClip,
    PSInStroke,
    PSInUEOFill,
    PSInUFill,
    PSInUStroke,
    PSInvertMtx,
    PSITfm,
    PSLineTo,
    PSMtx,
    PSMoveTo,
    NewPath,
    NullDevice,
    PSPathBBox,
    PSPathForAll,
    PSRCurveTo,
    PSRectClip,
    PSRectFill,
    PSRectStroke,
    PSRectViewClip,
    PSReversePath,
    PSRLineTo,
    PSRMoveTo,
    PSRtat,
    PSScal,
    PSSetBBox,
    PSSetBlkGeneration,
    PSSetCMYKColor,
    PSSetColorScreen,
    PSSetClrTransfer,
    PSSetDash,
    PSSetFlatThreshold,
    PSSetGray,
    PSSetGState,
    PSSetHalftone,
    PSSetHalftonePhase,
    PSSetHSBColor,
    PSSetLineCap,
    PSSetLineJoin,
    PSSetLineWidth,
    PSSetMtx,
    PSSetMiterLimit,
    PSSetRGBColor,
    PSSetScreen,
    PSSetStrokeAdjust,
    PSSetTransfer,
    PSSetUCacheParams,
    PSSetUCRemoval,
    PSShowPage,
    PSStroke,
    PSStrkPth,
    PSTfm,
    PSTlat,
    PSUAppend,
    NoOp,
    PSUCacheStatus,
    PSUEOFill,
    PSUFill,
    PSUPath,
    PSUStroke,
    PSUStrokePath,
    PSViewClip,
    PSViewClipPath,
    PSWTranslation,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_graphicsDPS, 120, 256*PACKAGE_INDEX + 1);
}
