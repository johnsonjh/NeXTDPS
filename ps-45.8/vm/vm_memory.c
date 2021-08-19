/*
  vm_memory.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Ivor Durham: Sun May  7 14:55:02 1989
Ed Taft: Sun Nov 26 15:40:25 1989
Jim Sandman: Tue Apr  4 17:29:09 1989
Paul Rovner: Tuesday, October 25, 1988 1:00:48 PM
Perry Caro: Mon Nov  7 11:41:11 1988
Joe Pasqua: Mon Jan  9 16:15:47 1989
End Edit History.
*/

/*
 * A VM consists of a sequence of segments.  There may be multiple VMs, only
 * one of which is shared.
 */

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include ORPHANS
#include PSLIB
#include STREAM
#include VM

#include "vm_space.h"
#include "vm_memory.h"
#include "vm_segment.h"

/* The following declarations are for procedures in the	*/
/* language pkg. language.h can't be imported - that	*/
/* would cause circular interface dependencies.		*/
extern	procedure PSPushInteger();

/*
 * State
 */

typedef struct {
  PVM	vmCurrent;
  PVM	vmPrivate;
} VM_Data, *PVM_Data;

private PVM_Data VM_Context_Data;

public	PVM vmShared;
public	PVM vmPrivate;
public	PVM vmCurrent;

public procedure SetShared (Shared)
  boolean Shared;
{
  if (Shared) {
    vmCurrent = vmShared;
    NOLL.level = 0;
  } else {
    vmCurrent = vmPrivate;
    NOLL.level = level;
  }
}

public PCard8 AppendVM(n, p)
  cardinal n;		/* number of bytes to append */
  PCard8  p;		/* ptr to bytes to append */
  /*
   * This function appends n bytes at p to virtual memory
   * and returns the offset where the bytes were appended.
   */
{
  PCard8  RESULT = AllocChars((integer)n),	/* allocate VM space */
          dest = RESULT;
  register cardinal i;		/* just a counter */

  /*
   * May want to use bcopy here, but apparenly never copies more than nine
   * bytes according to Ed.
   */

  for (i = 0; i < n; i++)
    *dest++ = *p++;		/* copy n bytes */

  return RESULT;
}				/* end of AppendVM */

#if	(STAGE == DEVELOP)
private procedure PSDisplayVM()
{
  os_fprintf (os_stderr, "Shared VM ");
  DisplayVMSection (vmShared);
  os_fprintf (os_stderr, "Private VM ");
  DisplayVMSection (vmPrivate);
}
#endif	(STAGE == DEVELOP)

public procedure PSVMStatus()
 /*
    -- vmstatus used maximum, with answers depending on "setshared"
  */
{
  Int32   Used,
          Size;

  PSPushInteger ((integer) level);
  VM_Usage (vmCurrent, &Used, &Size);
  PSPushInteger (Used);
  PSPushInteger (Size);
} /* end of PSVMStatus */

#if VMINIT
/* Following procs implement setram and setrom operators and are also
   exported to the vm interface */

public procedure VMSetRAMAlloc() {SetVMSegmentType(stPermanentRAM);}
public procedure VMSetROMAlloc() {SetVMSegmentType(stROM);}
#endif VMINIT

#define VMSTATICEVENTS \
  STATICEVENTFLAG(staticsCreate) | \
  STATICEVENTFLAG(staticsLoad) | \
  STATICEVENTFLAG(staticsUnload)

private procedure VM_Data_Handler (code)
  StaticEvent code;
{
  switch (code) {
   case staticsCreate:
    VM_Context_Data->vmPrivate = vmPrivate;
    VM_Context_Data->vmCurrent = vmCurrent;
    break;
   case staticsLoad:
    vmPrivate = VM_Context_Data->vmPrivate;
    vmCurrent = VM_Context_Data->vmCurrent;
    break;
   case staticsUnload:
    VM_Context_Data->vmCurrent = vmCurrent;
    break;
  }
}

Init_VM_Memory (reason)
  InitReason reason;
{
  switch (reason) {
   case init:
    RegisterData(
      (PCard8 *)&VM_Context_Data, (integer)sizeof(VM_Data), VM_Data_Handler,
      (integer)VMSTATICEVENTS);
    CreateSegmentPool ();
#if VMINIT
    vVMSPLIT = GetCSwitch("VMSPLIT", 0);
#endif VMINIT
    break;
   case romreg:
#if VMINIT
    if (vmShared->wholeCloth)
      {
      Begin(rootShared->vm.Shared.internalDict);
      RgstExplicit("setram", VMSetRAMAlloc);
      RgstExplicit("setrom", VMSetROMAlloc);
      End();
      }
#endif VMINIT
#if	(STAGE == DEVELOP)
    if (vSTAGE == DEVELOP)
      RgstExplicit ("displayvm", PSDisplayVM);
#endif	(STAGE == DEVELOP)
    break;
   endswitch
  }
}
