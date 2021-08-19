/*
  ops_vmDPS.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_vmDPS". It was generated automatically,
using information derived from the vmnames.h file.
*/

{
  extern void PSRstr();
  extern void PSSave();
  extern void PSSetThresh();
  extern void PSCollect();
  extern void PSVMStatus();
#if (OS == os_mpw)
  void (*ops_vmDPS[5])();
  register void (**p)() = ops_vmDPS;
  *p++ = PSRstr;
  *p++ = PSSave;
  *p++ = PSSetThresh;
  *p++ = PSCollect;
  *p++ = PSVMStatus;
#else (OS == os_mpw)
  static void (*ops_vmDPS[5])() = {
    PSRstr,
    PSSave,
    PSSetThresh,
    PSCollect,
    PSVMStatus,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_vmDPS, 5, 256*PACKAGE_INDEX + 1);
}
