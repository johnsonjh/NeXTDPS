/*
  ops_fontsDPS.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_fontsDPS". It was generated automatically,
using information derived from the fontsnames.h file.
*/

{
  extern void PSAShow();
  extern void PSAWidthShow();
  extern void PSCacheStatus();
  extern void PSCShow();
  extern void PSCrCParams();
  extern void PSCrFont();
  extern void PSDefineFont();
  extern void PSIntDict();
  extern void PSKShow();
  extern void PSMakeFont();
  extern void PSRootFont();
  extern void PSScaleFont();
  extern void PSSelectFont();
  extern void PSSetCchDevice();
  extern void PSSetCchDevice2();
  extern void PSSetCacheLimit();
  extern void PSStCParams();
  extern void PSSetCharWidth();
  extern void PSSetFont();
  extern void PSShow();
  extern void PSStrWidth();
  extern void PSUnDefineFont();
  extern void PSWidthShow();
  extern void PSXShow();
  extern void PSXYShow();
  extern void PSYShow();
#if (OS == os_mpw)
  void (*ops_fontsDPS[26])();
  register void (**p)() = ops_fontsDPS;
  *p++ = PSAShow;
  *p++ = PSAWidthShow;
  *p++ = PSCacheStatus;
  *p++ = PSCShow;
  *p++ = PSCrCParams;
  *p++ = PSCrFont;
  *p++ = PSDefineFont;
  *p++ = PSIntDict;
  *p++ = PSKShow;
  *p++ = PSMakeFont;
  *p++ = PSRootFont;
  *p++ = PSScaleFont;
  *p++ = PSSelectFont;
  *p++ = PSSetCchDevice;
  *p++ = PSSetCchDevice2;
  *p++ = PSSetCacheLimit;
  *p++ = PSStCParams;
  *p++ = PSSetCharWidth;
  *p++ = PSSetFont;
  *p++ = PSShow;
  *p++ = PSStrWidth;
  *p++ = PSUnDefineFont;
  *p++ = PSWidthShow;
  *p++ = PSXShow;
  *p++ = PSXYShow;
  *p++ = PSYShow;
#else (OS == os_mpw)
  static void (*ops_fontsDPS[26])() = {
    PSAShow,
    PSAWidthShow,
    PSCacheStatus,
    PSCShow,
    PSCrCParams,
    PSCrFont,
    PSDefineFont,
    PSIntDict,
    PSKShow,
    PSMakeFont,
    PSRootFont,
    PSScaleFont,
    PSSelectFont,
    PSSetCchDevice,
    PSSetCchDevice2,
    PSSetCacheLimit,
    PSStCParams,
    PSSetCharWidth,
    PSSetFont,
    PSShow,
    PSStrWidth,
    PSUnDefineFont,
    PSWidthShow,
    PSXShow,
    PSXYShow,
    PSYShow,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_fontsDPS, 26, 256*PACKAGE_INDEX + 1);
}
