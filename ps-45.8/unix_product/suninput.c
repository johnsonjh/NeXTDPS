/*
  suninput.c

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

Original version: Thu Dec  8 23:29:06 1983
Edit History:
Bill Paxton: Wed Oct 28 07:12:54 1987
Ed Taft: Fri Nov 24 16:27:41 1989
Doug Brotz: Mon Jun  2 10:34:03 1986
Chuck Geschke: Thu Oct 10 06:44:34 1985
Ivor Durham: Wed Jun 15 16:51:07 1988
Linda Gass: Fri Dec  4 15:41:35 1987
Jim Sandman: Wed Jun 15 09:10:39 1988
Joe Pasqua: Mon Jan 16 14:19:11 1989
End Edit History.

This entire module should be included only in configurations with
SUNTOOL==true.
*/

#include PACKAGE_SPECS
#include BASICTYPES
#include ERROR
#include LANGUAGE
#include STREAM
#include DEVICE
#include ORPHANS
#include PSLIB
#include GRAPHICS
#include VM


public Card8 vSUNTOOL;
extern	int toolwidth, toolheight;

private procedure psToolHeight() { PushInteger((integer)toolheight); }
private procedure psToolWidth() { PushInteger((integer)toolwidth); }

#if	0
private unsigned user_cursor_image[8];

mpr_static(user_cursor, 16, 16, 1, user_cursor_image);

private procedure psSetCursor() {
  struct cursor usercursor;
  int i;
  os_bzero((char *)&usercursor,sizeof (struct cursor));
  for (i=7; i >= 0; i--) { user_cursor_image[i] = PopInteger(); }
  usercursor.cur_xhot = hotspotx;
  usercursor.cur_yhot = hotspoty;
  usercursor.cur_function = PIX_SRC | PIX_DST;
  usercursor.cur_shape = &user_cursor;
  win_setcursor(toolsw->ts_windowfd, &usercursor);
  }

private procedure psSetHotSpot() {
  hotspoty = PopInteger();
  hotspotx = PopInteger(); }
#endif	0

private procedure DummyPop0()
{
}

private procedure DummyPop1()
{
  Object O;
  PopP(&O);
}

private procedure DummyPop2()
{
  Object O;
  PopP(&O);
  PopP(&O);
}

private procedure DummyPop3()
{
  Object O;
  PopP(&O);
  PopP(&O);
  PopP(&O);
}

#if	0
private procedure psSetCursorPosition()
{
  integer cx, cy;
  cy = PopInteger(); cx = PopInteger();
  win_setmouseposition(toolsw->ts_windowfd, cx, cy);
}
#endif	0

extern procedure psUndoMoreCursor (), psArrowCursor (), PSSetWindow ();

private readonly RgCmdTable cmdSunInput = {
/*  "SetCursorPosition", psSetCursorPosition, */
/*  "AbortInput", Abort, */
/*  "SetInputAbort", psSetInputAbort, */
/*  "TTYReadLine", psTTYReadLine, */
  "ToolHeight", psToolHeight,
  "ToolWidth", psToolWidth,
  "UndoMoreCursor", psUndoMoreCursor,
  "ArrowCursor", psArrowCursor,
/*  "SetCursor", psSetCursor, */
/*  "SetHotSpot", psSetHotSpot, */
/*  "CloseWindow", psCloseWindow, */
/*  "OpenWindow", psOpenWindow, */
/*  "bye", psExitTool, */
  "CancelButton", DummyPop0,
  "LeftDown", DummyPop2,
  "LeftMove", DummyPop2,
  "LeftUp", DummyPop2,
  "CtrlLeftDown", DummyPop2,
  "CtrlLeftMove", DummyPop2,
  "CtrlLeftUp", DummyPop2,
  "ShiftLeftDown", DummyPop2,
  "ShiftLeftMove", DummyPop2,
  "ShiftLeftUp", DummyPop2,
  "CtrlShiftLeftDown", DummyPop2,
  "CtrlShiftLeftMove", DummyPop2,
  "CtrlShiftLeftUp", DummyPop2,
  "MiddleDown", DummyPop2,
  "MiddleMove", DummyPop2,
  "MiddleUp", DummyPop2,
  "CtrlMiddleDown", DummyPop2,
  "CtrlMiddleMove", DummyPop2,
  "CtrlMiddleUp", DummyPop2,
  "ShiftMiddleDown", DummyPop2,
  "ShiftMiddleMove", DummyPop2,
  "ShiftMiddleUp", DummyPop2,
  "CtrlShiftMiddleDown", DummyPop2,
  "CtrlShiftMiddleMove", DummyPop2,
  "CtrlShiftMiddleUp", DummyPop2,
  "RightDown", DummyPop2,
  "RightMove", DummyPop2,
  "RightUp", DummyPop2,
  "CtrlRightDown", DummyPop2,
  "CtrlRightMove", DummyPop2,
  "CtrlRightUp", DummyPop2,
  "ShiftRightDown", DummyPop2,
  "ShiftRightMove", DummyPop2,
  "ShiftRightUp", DummyPop2,
  "CtrlShiftRightDown", DummyPop2,
  "CtrlShiftRightMove", DummyPop2,
  "CtrlShiftRightUp", DummyPop2,
  "LeftDownOnButton", DummyPop1,
  "RightDownOnButton", DummyPop1,
  "DoButton", DummyPop3,   
  "PostAbort", DummyPop0,
  "setwindow", PSSetWindow,
  NIL};

public procedure IniSunInput(reason) InitReason reason;
{
  switch (reason) {
   case init:
    break;
   case romreg:
    break;
   case ramreg:
    vSUNTOOL = GetCSwitch("SUNTOOL", 0);
    if (vSUNTOOL) {
      RgstMCmds (cmdSunInput);
    }
    break;
    endswitch
  }
}				/* end of IniSunInput */
