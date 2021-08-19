/*
  ops_fontsInternal.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_fontsInternal". It was generated automatically,
using information derived from the fontsnames.h file.
*/

{
  extern void PSCCRun();
  extern void PSeCCRun();
  extern void PSErodeSW();
  extern void PSFontRun();
  extern void PSLck();
  extern void StartLock();
  extern void PSSetXLock();
  extern void PSSetYLock();
#if (OS == os_mpw)
  void (*ops_fontsInternal[8])();
  register void (**p)() = ops_fontsInternal;
  *p++ = PSCCRun;
  *p++ = PSeCCRun;
  *p++ = PSErodeSW;
  *p++ = PSFontRun;
  *p++ = PSLck;
  *p++ = StartLock;
  *p++ = PSSetXLock;
  *p++ = PSSetYLock;
#else (OS == os_mpw)
  static void (*ops_fontsInternal[8])() = {
    PSCCRun,
    PSeCCRun,
    PSErodeSW,
    PSFontRun,
    PSLck,
    StartLock,
    PSSetXLock,
    PSSetYLock,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_fontsInternal, 8, 256*PACKAGE_INDEX + 3);
}
