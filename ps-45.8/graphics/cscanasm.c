/* Get next non-xtra Cross in PathForw direction */
private CrossPtr ForwPathCross(cp)
  CrossPtr cp;
  {
  REG CrossPtr c;

  c = cp + 1;
  while (true) {
    if (!c->c.f.xtra)
      return c;
    if (c->c.f.link)
      c = c->l.forw;			/* Follow connection */
    else
      c++;
    }
  }

/* Get previous non-xtra Cross in PathBack direction */
private CrossPtr BackPathCross(cp)
  CrossPtr cp;
  {
  REG CrossPtr c;

  c = cp - 1;
  while (true) {
    if (!c->c.f.xtra)
      return c;
    if (c->c.f.link)
      c = c->l.back;			/* Follow connection */
    else
      c--;
    }
  }


/* Get next Cross in given direction */
private CrossPtr PathXtraCross(cp, dir)
  CrossPtr cp;
  PathDir dir;
  {
  REG CrossPtr c;

  DEBUGERROR(dir!=PathForw && dir!=PathBack, "PathXtraCross: Invalid path direction");

  if (dir == PathForw) {
    c = cp + 1;
    if (c->c.f.link)
      c = c->l.forw;
    }
  else {
    c = cp - 1;
    if (c->c.f.link)
      c = c->l.back;
    }

  return c;
  }


/* Return the Cross in the "up" direction from the specified Cross */
private CrossPtr PathUpCross(cross)
  CrossPtr cross;
  {
  if (PathXtraCross(cross,PathForw)->c.y > cross->c.y)
    return ForwPathCross(cross);
  else
    return BackPathCross(cross);
  }


/* Get next xtra Cross in given direction *from original path* */
private CrossPtr PathXtraOrig(cp, dir)
  CrossPtr cp;
  PathDir dir;
  {
  REG CrossPtr c;

  c = cp;
  while (true) {
    c = PathXtraCross(c, dir);
    if (!c->c.f.splice)
      return c;
    }
  }


/* Offset center computation - these are actually cotangents not tangents I think */
/*
	theta	tantheta(hex)	tantheta	offset(hex)	offset
0	<no offset> ...
1	7.5	21B4		0.131652	FFFFF041	-0.0615118
2	15.0	4498		0.267949	FFFFE238	-0.116337
3	22.5	6A0A		0.414214	FFFFD587	-0.165911
4	30.0	93CD		0.57735		FFFFC9E7	-0.211325
5	37.5	C470		0.767327	FFFFBF1F	-0.253427
6	45.0	10000		1.0		FFFFB505	-0.292893
7	52.5	14DA0		1.30323		FFFFAB73	-0.330273
8	60.0	1BB68		1.73205		FFFFA24C	-0.366025
9	67.5	26A0A		2.41421		FFFF9976	-0.400544
10	75.0	3BB68		3.73205		FFFF90DA	-0.434174
11	82.5	79883		7.59575		FFFF8864	-0.467228
12	90.0					-FixedHalf

/offsetcenter {
  /oc exch def
  /dx2 oc oc mul def
  /b dx2 dx2 1 add div sqrt def
  /oc 1 b sub 1 b add div sqrt 1 sub .5 mul def
  oc
  } def

/cvrhexprint { 65536 mul round 16 (                      ) cvrs print} def

7.5 7.5 89 {
  /theta exch def
  theta (     ) cvs print ( ) print
  /tantheta theta sin theta cos div def
  tantheta cvrhexprint ( ) print
  tantheta (                     ) cvs print (   ) print
  tantheta offsetcenter dup cvrhexprint ( ) print =
  } for

*/

/* Return the index into the offset-center offset table for a given slope */
private SlopeNumber OffsetCenterSlope(xDelta)
  Fixed xDelta;
  {
  REG Fixed tempDx;

  tempDx = xDelta;
  if (tempDx < 0) tempDx = -tempDx;

  /* tempDx is tan(theta) */
  if (tempDx >= 0x14DA0) {		/* theta >= 52.5 */
    if (tempDx >= 0x79883)		/* theta >= 82.5 */
      return SLOPE90;
    else if (tempDx >= 0x26A0A)		/* theta >= 67.5 */
      return SLOPE75;
    else
      return SLOPE60;
    }
  else if (tempDx <= 0x21B4)		/* theta <= 7.5 */
    return SLOPE00;
  else
    return SLOPE45;			/* Lock smallish slopes to 45 */
  }


/*
 * Build cross data structures given an array of points.
 * (The array has been constructed by repeated calls to CSNewPoint().)
 * Returns true if any non-xtra Crosses were created.
 */
private procedure BuildCrosses(points, count, allPoints, offset)
  PFCd points;			/* Array of points */
  REG IntX count;		/* # of points in array */
  boolean allPoints;
  boolean offset;		/* Do offset-center algorithm */
  {
  REG PFCd start, end;		/* Start and end of the current segment */
  Fixed yStartMid;		/* Y-start moved toward end to midline */
  Fixed xDelta;			/* X/Y slope -- X delta per unit Y */
  Fixed curX;			/* Current X value */
  REG CrossPtr c1;		/* Current Cross */
  REG CrossPtr prevc;		/* Previous Cross */
  SegmentDir vertDir;		/* Previous vertical direction */
  SegmentDir horizDir;		/* Previous horizontal direction */
  boolean haveCross;		/* True = Path has Crosses */


  /*
   * Define the first segment from the last point of the previous
   * array and the first of the new array.
   * Caller (CSPathPoints()) handles case of no previous array.
   */
  start = &savePrevPoint;
  end = points;
  vertDir = saveVertDir;
  horizDir = saveHorizDir;
  prevc = savePrevCross;

  haveCross = false;		/* Do not want "saved" version of this */

  /* Make sure we never have a point *exactly* on a midline */
  if ((start->y & 0xFFFFL) == 0x8000)
    start->y += 1;

  while(--count >= 0) {
    /* Make sure we never have a point *exactly* on a midline */
    if ((end->y & 0xFFFFL) == 0x8000)
      end->y += 1;

#if GRAPHCHAR || GRAPHPOINTS
    {
    float f1, f2;

    fixtopflt(start->x, &f1);
    fixtopflt(start->y, &f2);
    printf("%g %g RP\n", f1, f2);		/* x y RP  == One raw point in a path */
    }
#endif  /* GRAPHCHAR */

    c1 = NULL;

    /* If we are recording all points, make an xtra Cross for this one */
    if (allPoints)
      c1 = NewXtraCross(start->x, start->y);

    /* If x inflection point, generate an xtra Cross for it */
    if (start->x <= end->x) {
      if (start->x != end->x) {			/* Path going RIGHT */
	if (horizDir != RightDir) {
	  if (start->x < xPathMin)
	    xPathMin = start->x;
	  if (c1 == NULL)
	    c1 = NewXtraCross(start->x, start->y);
	  c1->c.yNext = pathMinX;  pathMinX = c1;	/* Add to minimum list */
	  horizDir = RightDir;
	  }
	}
      else {					/* Path VERTICAL */
	if (horizDir != VertDir) {
	  if (c1 == NULL)
	    c1 = NewXtraCross(start->x, start->y);
	  if (horizDir == LeftDir)
	    { c1->c.yNext = pathMinX;  pathMinX = c1; }	/* Add to minimum list */
	  else
	    { c1->c.yNext = pathMaxX;  pathMaxX = c1; }	/* Add to maximum list */
	  horizDir = VertDir;
	  }
	}
      }
    else {					/* Path going LEFT */
      if (horizDir != LeftDir) {
	if (start->x > xPathMax)
	  xPathMax = start->x;
	if (c1 == NULL)
	  c1 = NewXtraCross(start->x, start->y);
	c1->c.yNext = pathMaxX;  pathMaxX = c1;		/* Add to maximum list */
	horizDir = LeftDir;
	}
      }

    if (start->y <= end->y) {
      if (start->y != end->y) {			/* Path going UP */
	if (vertDir != UpDir) {			/* Bottom of Y curve */
	  if (start->y < yPathMin) {
	    yPathMin = start->y;
	    }
	  if (c1 == NULL)
	    NewXtraCross(start->x, start->y);
	  }
	vertDir = UpDir;
	yStartMid = ((start->y + FixedHalf) & 0xFFFF0000L) | 0x8000;
	if (yStartMid > end->y)
	  goto NEXT;
	xDelta = fixdiv(end->x-start->x, end->y-start->y);
	curX = start->x + fixmul(xDelta, yStartMid-start->y);
	c1 = NewCross(curX, yStartMid);
	if (offset)
	  c1->c.f.delta = OffsetCenterSlope(xDelta);
	haveCross = true;
	if (c1->c.y > prevc->c.y)
	  { c1->c.f.down = PathBack;  prevc->c.f.up = true; }
	else if (c1->c.y < prevc->c.y)
	  { prevc->c.f.down = PathForw;  c1->c.f.up = true; }
	else if (c1->c.x < prevc->c.x)		/* Must be horizontal connection */
	  c1->c.f.hright = PathBack;
	else
	  prevc->c.f.hright |= PathForw;	/* Previous could have connection already */
	while (true) {
	  prevc = c1;
	  yStartMid += FixedOne;
	  if (yStartMid > end->y) break;
	  curX += xDelta;
	  c1 = NewCross(curX, yStartMid);
	  if (offset)
	    c1->c.f.delta = OffsetCenterSlope(xDelta);
	  c1->c.f.down = PathBack;
	  prevc->c.f.up = true;
	  }
	}
      else {					/* Path HORIZONTAL. Never on midline. */
	if (vertDir != HorizDir && c1 == NULL)
	  NewXtraCross(start->x, start->y);
	vertDir = HorizDir;
	}
      }
    else {					/* Path going DOWN */
      if (vertDir != DownDir) {			/* Top of y curve */
	if (start->y > yPathMax)
	  yPathMax = start->y;
	if (c1 == NULL)
	  NewXtraCross(start->x, start->y);
	}
      vertDir = DownDir;
      yStartMid = ((start->y - FixedHalf) & 0xFFFF0000L) | 0x8000;
      if (yStartMid < end->y)
	goto NEXT;
      xDelta = fixdiv(end->x-start->x, end->y-start->y);
      curX = start->x + fixmul(xDelta, yStartMid-start->y);
      c1 = NewCross(curX, yStartMid);
      if (offset)
	c1->c.f.delta = OffsetCenterSlope(xDelta);
      haveCross = true;
      if (c1->c.y < prevc->c.y)
	{ prevc->c.f.down = PathForw;  c1->c.f.up = true; }
      else if (c1->c.y > prevc->c.y)
	{ c1->c.f.down = PathBack;  prevc->c.f.up = true; }
      else if (c1->c.x < prevc->c.x)	/* Must be horizontal connection */
	c1->c.f.hright = PathBack;
      else
	prevc->c.f.hright |= PathForw;	/* Previous could have connection already */
      while (true) {
	prevc = c1;
	yStartMid -= FixedOne;
	if (yStartMid < end->y) break;
	curX -= xDelta;
	c1 = NewCross(curX, yStartMid);
	if (offset)
	  c1->c.f.delta = OffsetCenterSlope(xDelta);
	prevc->c.f.down = PathForw;
	c1->c.f.up = true;
	}
      }

    NEXT:
    start = end++;
    }

  havePathCross |= haveCross;

  saveVertDir = vertDir;
  saveHorizDir = horizDir;
  savePrevCross = prevc;
  savePrevPoint = *start;
  }


/* New path point */
public procedure CSNewPoint(pc)
  PFCd pc;
  {
  if (pointCount == POINT_ARRAY_LEN) {
    CSPathPoints(pointArray, pointCount, false);
    pointCount = 0;
    }
  pointArray[pointCount++] = *pc;
  }


/* Return the left hand of the pair that the specified Cross is in */
private CrossPtr RunPair(cross)
  CrossPtr cross;
  {
  REG CrossPtr cl;		/* Left hand Cross */
  REG CrossPtr cr;		/* Right hand Cross */

  if (cross->c.f.isLeft)
    return cross;

  cl = YCROSS(Pixel(cross->c.y));
  while(true) {
    DEBUGERROR(cl==NULL,"RunPair: Couldn't find specified cross");
    cr = cl->c.yNext;
    DEBUGERROR(cr==NULL,"RunPair: Odd number of Crosses in scanline");
    if (cr == cross)
      return cl;
    cl = cr->c.yNext;
    }
  }


/* Return the x midline intercept of a line segment determined by two Crosses.
   WARNING:  Note that the x midline value is a *Fixed*.  */
private Fixed YCrossing(c1, c2, xval)
  REG CrossPtr c1, c2;		/* The two Crosses that define the line segment */
  Fixed xval;
  {
  return c1->c.y + fixmul(xval-c1->c.x, fixdiv(c2->c.y-c1->c.y, c2->c.x-c1->c.x));
  }
