/*
  ops_postscriptDPS.c

THIS IS A DERIVED FILE -- DO NOT EDIT IT

This is the program fragment for registering the operator set
named "ops_postscriptDPS". It was generated automatically,
using information derived from the postscriptnames.h file.
*/

{
  extern void PSCondition();
  extern void PSCurrentContext();
  extern void PSCurrentShared();
  extern void PSDetach();
  extern void PSFork();
  extern void PSJoin();
  extern void PSLock();
  extern void PSMonitor();
  extern void PSNotify();
  extern void PSQuit();
  extern void PSRealTime();
  extern void PSSetShared();
  extern void PSUserTime();
  extern void PSWait();
  extern void YieldOp();
#if (OS == os_mpw)
  void (*ops_postscriptDPS[15])();
  register void (**p)() = ops_postscriptDPS;
  *p++ = PSCondition;
  *p++ = PSCurrentContext;
  *p++ = PSCurrentShared;
  *p++ = PSDetach;
  *p++ = PSFork;
  *p++ = PSJoin;
  *p++ = PSLock;
  *p++ = PSMonitor;
  *p++ = PSNotify;
  *p++ = PSQuit;
  *p++ = PSRealTime;
  *p++ = PSSetShared;
  *p++ = PSUserTime;
  *p++ = PSWait;
  *p++ = YieldOp;
#else (OS == os_mpw)
  static void (*ops_postscriptDPS[15])() = {
    PSCondition,
    PSCurrentContext,
    PSCurrentShared,
    PSDetach,
    PSFork,
    PSJoin,
    PSLock,
    PSMonitor,
    PSNotify,
    PSQuit,
    PSRealTime,
    PSSetShared,
    PSUserTime,
    PSWait,
    YieldOp,
  };
#endif (OS == os_mpw)
  RgstOpSet(ops_postscriptDPS, 15, 256*PACKAGE_INDEX + 1);
}
