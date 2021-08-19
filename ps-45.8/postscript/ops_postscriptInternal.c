/*
  ops_postscriptInternal.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_postscriptInternal". It was generated automatically,
using information derived from the postscriptnames.h file.
*/

{
  extern void PSClrInt();
  extern void DisableCC();
  extern void EnableCC();
#if (OS == os_mpw)
  void (*ops_postscriptInternal[3])();
  register void (**p)() = ops_postscriptInternal;
  *p++ = PSClrInt;
  *p++ = DisableCC;
  *p++ = EnableCC;
#else (OS == os_mpw)
  static void (*ops_postscriptInternal[3])() = {
    PSClrInt,
    DisableCC,
    EnableCC,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_postscriptInternal, 3, 256*PACKAGE_INDEX + 2);
}
