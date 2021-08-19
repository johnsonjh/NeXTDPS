/*
  vm_startfile.c

Copyright (c) 1984, '86, '87, '88, '89 Adobe Systems Incorporated.
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
Ivor Durham: Sat Oct 15 14:10:34 1988
Jim Sandman: Wed Apr  5 09:49:10 1989
Joe Pasqua: Mon Jan  9 16:08:05 1989
Ed Taft: Sat Nov 25 11:19:24 1989
End Edit History.

Procedures to start up an existing VM by reading it from a file
*/

/*
 * Layout of VM file:
 *
 *	1. VM Shared section
 *	   a. VMStructure header
 *	   b. Sequence of {VMSegment header, VM data} for each segment
 *	2. Relocation table.
 *	3. Switches
 *
 * In reading a VM from a file, new segments are constructed in exactly the
 * same size and order as those in the file. We then read the relocation
 * table, perform the fixups specified in it, and discard it.
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

private boolean ReadVM(PnewVM, file)
  PVM *PnewVM;
  Stm file;
 /*
  * Build new VM from file, discarding previous one, if any.
  */
{
  boolean ok;
  integer size,
          used;
  PVM     vmTarget = *PnewVM;
  VMStructure tempVM;
  PVMSegment nextSegment,
          newSegment;
  VMSegment tempSegment;
  Level   level;

  size = sizeof (VMStructure);

  ok = (fread (&tempVM, 1, size, file) == size);

  if (ok) {
    if (tempVM.password != VMpassword) {
      os_eprintf ("Invalid password in VM file.\n");
      ok = false;
    }
  } else {
    os_eprintf ("Failed to read shared VM header\n");
  }

  if (ok) {
    DestroyVM (vmTarget);	/* Discard previous one */
    *PnewVM = vmTarget = CreateVM (true, false);
  }

  do {
    ok= (fread (&tempSegment, 1, sizeof (VMSegment), file) == sizeof (VMSegment));

    if (ok) {
      size = VMSegmentSize (&tempSegment);
      used = tempSegment.free - tempSegment.first;
      level = tempSegment.level;

      vmTarget->current = NIL;	/* Force expansion with exact size */

      ExpandVMSection (vmTarget, (PCard8) NIL, size);

      ok = (fread (vmTarget->free, 1, used, file) == used);

      vmTarget->free = vmTarget->current->free = vmTarget->current->first + used;
      vmTarget->current->level = level;
    }
  } while (ok && (tempSegment.succ != NIL));

  vmTarget->free = tempVM.free; /* this will be relocated by caller */

  return (ok);
}

public boolean StartVM(file)
  Stm     file;
 /*
  * Read previously saved VM from given Stm.  Leaves Stm open, but the
  * position is unspecified.
  */
{
  boolean ok;

  if (file == NIL) return false;

  ok = ReadVM (&vmShared, file);

  if (ok) {
    ok = ReadRelocationTable (file);
  }

  if (ok) {
    BuildSegmentTable((PCard8) NIL, (PCard8) NIL);
    ApplyRelocation();
    FreeSegmentTable();
    ok = (fread (&switches, 1, sizeof (Switches), file) == sizeof (Switches));
  }

  if (!ok) {
    os_fprintf (os_stderr, "File error while attempting to read VM!");
    return false;
  }

  SetShared (true);	/* Re-visit this choice */
#if VMINIT
  VMSetRAMAlloc();	/* switch to stPermanentRAM if building VMSPLIT */
#endif VMINIT
  return true;
} /* ReadVMFromFile */
