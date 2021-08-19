/*
  ops_fonts2.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_fonts2". It was generated automatically,
using information derived from the fontsnames.h file.
*/

{
  extern void PSSelectFont();
  extern void PSUnDefineFont();
  extern void PSXShow();
  extern void PSXYShow();
  extern void PSYShow();
  extern void PSCShow();
  extern void PSRootFont();
  extern void PSSetCchDevice2();
#if (OS == os_mpw)
  void (*ops_fonts2[8])();
  register void (**p)() = ops_fonts2;
  *p++ = PSSelectFont;
  *p++ = PSUnDefineFont;
  *p++ = PSXShow;
  *p++ = PSXYShow;
  *p++ = PSYShow;
  *p++ = PSCShow;
  *p++ = PSRootFont;
  *p++ = PSSetCchDevice2;
#else (OS == os_mpw)
  static void (*ops_fonts2[8])() = {
    PSSelectFont,
    PSUnDefineFont,
    PSXShow,
    PSXYShow,
    PSYShow,
    PSCShow,
    PSRootFont,
    PSSetCchDevice2,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_fonts2, 8, 256*PACKAGE_INDEX + 2);
}
