/*
  ops_fonts1.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_fonts1". It was generated automatically,
using information derived from the fontsnames.h file.
*/

{
  extern void PSAShow();
  extern void PSAWidthShow();
  extern void PSCacheStatus();
  extern void PSCrCParams();
  extern void PSCrFont();
  extern void PSDefineFont();
  extern void PSIntDict();
  extern void PSKShow();
  extern void PSMakeFont();
  extern void PSScaleFont();
  extern void PSSetCchDevice();
  extern void PSSetCacheLimit();
  extern void PSStCParams();
  extern void PSSetCharWidth();
  extern void PSSetFont();
  extern void PSShow();
  extern void PSStrWidth();
  extern void PSWidthShow();
#if (OS == os_mpw)
  void (*ops_fonts1[18])();
  register void (**p)() = ops_fonts1;
  *p++ = PSAShow;
  *p++ = PSAWidthShow;
  *p++ = PSCacheStatus;
  *p++ = PSCrCParams;
  *p++ = PSCrFont;
  *p++ = PSDefineFont;
  *p++ = PSIntDict;
  *p++ = PSKShow;
  *p++ = PSMakeFont;
  *p++ = PSScaleFont;
  *p++ = PSSetCchDevice;
  *p++ = PSSetCacheLimit;
  *p++ = PSStCParams;
  *p++ = PSSetCharWidth;
  *p++ = PSSetFont;
  *p++ = PSShow;
  *p++ = PSStrWidth;
  *p++ = PSWidthShow;
#else (OS == os_mpw)
  static void (*ops_fonts1[18])() = {
    PSAShow,
    PSAWidthShow,
    PSCacheStatus,
    PSCrCParams,
    PSCrFont,
    PSDefineFont,
    PSIntDict,
    PSKShow,
    PSMakeFont,
    PSScaleFont,
    PSSetCchDevice,
    PSSetCacheLimit,
    PSStCParams,
    PSSetCharWidth,
    PSSetFont,
    PSShow,
    PSStrWidth,
    PSWidthShow,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_fonts1, 18, 256*PACKAGE_INDEX + 1);
}
