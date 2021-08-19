/*
  reducer.h

Copyright (c) 1984, '85, '86 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz, April 11, 1983
Edit History:
Doug Brotz: Tue May  6 10:33:11 1986
Chuck Geschke: Wed Nov 16 12:04:24 1983
Ivor Durham: Tue Dec  2 14:54:54 1986
Jim Sandman: Mon Sep 25 13:51:56 1989
End Edit History.
*/

#ifndef REDUCER_H
#define REDUCER_H

#include BASICTYPES

#define _reducer extern
#define _qreducer extern

             /* *********************************
             *                                  *
             *            Functions             *
             *                                  *
             ********************************* */


_reducer procedure ResetReducer();
/* Resets all state in the reducer package.  This function should
   be called before starting a new set of polygons to be reduced to
   trapezoids.
   ResetReducer reinitializes the reducer's storage and erases all
   memory of any previous paths given to it to reduce. */


_reducer procedure RdcClip( /* b (boolean) */ );
/* The polygons traced by the NewPoint function may be either "figure"
   polygons or "clip" polygons.  Figure polygons describe the shape to be
   output; clip polygons form a "mask" through which the figure polygons
   must pass.  The RdcClip function sets a state in the reducer so
   that subsequent polygons input through the NewPoint function are clip
   polygons (if the parameter "b" is true) or so that subsequent polygons
   will be figure polygons (if the parameter "b" is false).  A call to
   ResetReducer sets this state to false, i.e., figure polygons are the
   default polygon type.  See also the comment under the Reduce function
   for further description of the interaction between figure and clip
   polygons. */


_reducer procedure NewPoint( /* integer x, y; */ );
/* Adds a point at position  to the current polygon.  The reducer
   will connect this point to the previous point in the polygon with
   a straight line segment (i.e., like a lineto).  No line is drawn
   if this point is the first call to NewPoint after a ResetReducer call
   or a ClosePath call.
   Both x and y must be integers in the range [-32,000 .. 32,000]. */


_reducer procedure RdcClose();
/* Closes the current polygon by drawing a straight line segment from the
   last point entered via a call to NewPoint to the first point in the
   current polygon (i.e., the first call to NewPoint after a ResetReducer
   call or a former RdcClose call).  If no points have been entered
   since the last ResetReducer or RdcClose call, RdcClose
   does nothing.  If only one point has been entered on the current polygon,
   RdcClose closes up the degenerate polygon at that point
   properly. */


_reducer procedure Reduce( /* callBack, clipInterior, eoRule */ );
/* Converts the set of polygons input so far into a set of
   trapezoids that cover the "inside" of the set of polygons.  The second
   parameter, clipInterior, is a boolean value that determines how the set
   of clip polygons affects the trapezoids output for the "inside" of the
   figure polygons.  If "clipInterior" is true, then only the portion
   of the figure polygons that also lie inside the clip polygons is
   output as trapezoids.  If "clipInterior" is false, then only the
   portion of the figure trapezoids that lie outside the clip polygons
   is output as trapezoids.  In either case, only interior trapezoids
   of thefigure polygons are considered for output.  If no clip polygons
   have been input, then "clipInterior" should be false; otherwise no
   trapezoid output will result.  The last parameter, "eoRule" is a boolean
   that determines trapezoid output occurs for figure regions with non-zero
   winding number (eoRule=false), or if trapezoid output occurs for figure
   regions with odd winding number (eoRule=true).  Clipping region winding
   number output does not depend on this parameter, it always uses the
   non-zero rule.

   The first parameter, callBack, has the declaration

     boolean (*callBack)(),

   that is, it is a pointer to a function that returns a boolean value,
   and it takes parameters yt, yb, xtl, xtr, xbl, xbr (standing respectively
   for y-top, y-bottom, x-top-left, x-top-right, x-bottom-left, and
   x-bottom-right.  All these parameters to "callBack" are reals.
   "callBack" must return a boolean value: if true, Reduce will quit;
   if false, Reduce will continue.
   Reduce calls the "callBack" pointer-to-function parameter once for
   each output trapezoid.  After a call to Reduce has completed, the
   reducer package automatically resets (i.e., it calls ResetReducer
   automatically upon completion.)
   The "inside" of the set of polygons is determined by the winding
   number of each region formed by the interections of these polygons
   with themselves and with each other.  A zero winding number indicates
   "outside", while a non-zero winding number indicates "inside".  A
   polygon whose outside points are connected in a counter-clockwise
   direction has an interior whose winding number is one greater than
   its exterior, and vice-versa for a clockwise connected polygon.
   Thus, two concentric circles, one of which is clockwise and the other
   of which is connected counter-clockwise, yield a doughnut-shaped
   "inside", while two concentric circles, both of which are connected in
   the same direction, yield a solid interior up to the boundary of the
   larger circle.  */

_qreducer procedure QReduce
  ( /* boolean eofill; procedure (*callBack)(PRunAr pra); */ );
  /* "Scan-line reducer" that produces a list of runs for each scan line,
     rather than the individual trapezoids produced by the other reducer.
     This reducer is suitable for calling only if there is no clipping.
     Its performance is generally superior to the other reducer
     for small (dy of figure bounding box less than ~600) size figures or for
     very complex shapes. */

_qreducer procedure QResetReducer();

_qreducer procedure QNewPoint( /* Cd c; */ );

_qreducer procedure QRdcClose();

_qreducer procedure IniQReducer ( /* InitReason reason; */ );
extern procedure IniCScan(/* InitReason reason; */);

#define LENBUFF0 48000
#define LENBUFF1 24000
#define LENBUFF2 12000
#define LENBUFF3 4800


#endif REDUCER_H
