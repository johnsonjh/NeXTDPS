/*
  vm_startrom.c

Copyright (c) 1989 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Tue Nov 21 11:55:06 1989
Edit History:
Ed Taft: Sat Nov 25 17:50:33 1989
End Edit History.

Procedures to start up an existing VM that has been bound into ROM
*/

/*
  Assumptions:

  1. stROM segment starts at _vmROM and has been relocated to that address.
  2. Space for the stPermanentRAM segment has been statically allocated
     at _vmPermanentRAM, and the segment has been reloated to that address.
  3. A "VM file" starts at _vmFileData.

  Layout of VM file (note that this is similar to a VM actually saved
  as a file, omitting the ROM segment data and the relocation table).

  1. VM Shared section
     a. VMStructure header
     b. VMSegment header and VM data for stPermanentRAM segment
     c. VMSegment header (but not VM data) for stROM segment
     d. VMSegment header for (empty) stVolatileRAM segment
  2. Switches
 */

#include PACKAGE_SPECS
#include BASICTYPES
#include ENVIRONMENT
#include ERROR
#include EXCEPT
#include STREAM
#include VM

#include "vm_space.h"
#include "vm_memory.h"
#include "vm_segment.h"
#include "vm_relocate.h"

extern Card8 vmROM, vmPermanentRAM, vmFileData;

/* "VM Stream" */
/* a readable string stream of indefinite length */

private int VMStmFClose(stm) Stm stm; {StmDestroy(stm); return 0;} 

private StmProcs vmStmProcs = {
  StmErr, StmErr, StmFRead, StmZeroLong, StmUnGetc, StmErr,
  VMStmFClose, StmErr, StmErr, StmErr, StmErr, StmErrLong,
  "VMStm"};

private Stm VMStmCreate(str)
  char *str;
{
  Stm stm = StmCreate(&vmStmProcs, (integer) 0);
  stm->base = stm->ptr = str;
  stm->cnt = MAXInt32;
  stm->flags.read = 1;
  return stm;
}

/* end of VM Stream */

private procedure ReadVM(PnewVM, file)
  PVM *PnewVM;
  Stm file;
{
  integer size,
          used;
  PVM     vmTarget;
  VMStructure tempVM;
  PVMSegment nextSegment,
          newSegment;
  VMSegment tempSegment;
  Level   level;

  size = sizeof (VMStructure);

  if (fread (&tempVM, 1, size, file) != size ||
      tempVM.password != VMpassword)
    CantHappen();

  *PnewVM = vmTarget = CreateVM (true, false);
  Assert(vmTarget != NIL);

  do {
    if (fread (&tempSegment, 1, sizeof (VMSegment), file) != sizeof (VMSegment))
      CantHappen();

    size = VMSegmentSize (&tempSegment);
    used = tempSegment.free - tempSegment.first;
    level = tempSegment.level;

    vmTarget->current = NIL;	/* Force expansion with exact size */

    switch (level)
      {
      case stROM:
        Assert(tempSegment.first == &vmROM);
	ExpandVMSection(vmTarget, &vmROM, size);
        break;

      case stPermanentRAM:
        Assert(tempSegment.first == &vmPermanentRAM);
	ExpandVMSection(vmTarget, &vmPermanentRAM, size);
        if (fread (vmTarget->current->first, 1, used, file) != used)
	  CantHappen();
	break;

      case stVolatileRAM:
        ExpandVMSection(vmTarget, (PCard8) NIL, size);
        if (fread (vmTarget->current->first, 1, used, file) != used)
	  CantHappen();
        break;

      default:
        CantHappen();
      }

    vmTarget->free = vmTarget->current->free = vmTarget->current->first + used;
    vmTarget->current->level = level;
  } while (tempSegment.succ != NIL);

}

public boolean StartVM(file)
  Stm     file;
/* Note: this implementation ignores the file argument and instead
   starts up the VM that is bound into the code.
 */
{
  file = VMStmCreate(&vmFileData);
  Assert (file != NIL);

  ReadVM (&vmShared, file);

  rootShared = RootPointer(vmShared);

  if (fread (&switches, 1, sizeof (Switches), file) != sizeof (Switches))
    CantHappen();

  fclose(file);

  SetShared (true);	/* Re-visit this choice */
  return true;
} /* ReadVMFromFile */
