/* global_color.h */

#define MAXBANDS 32	/* Maximum number of bands. */
#define MAXBEDGE MAXBANDS*2   /* maximum number of band edges */
#define NDIFF ((MAXBANDS*MAXBANDS + MAXBANDS)/2)  /* max number of counters. */

typedef struct _gclr { /* stem */
  struct _gclr *next;
  Fixed w; /* desired device width */
  /* next and w fields must be first to match GCList */
  boolean yflg:1; /* true if this is y axis coloring info */
  boolean anchored:1; /* true if this pair cannot be moved */
  boolean active:1; /* true if this pair is currently enabled */
  boolean stdwidth:1; /* true if this pair has been set to a standard width */
  boolean alignable:1;
  BitField unused:11;
  Fixed cf; /* character coord for ideal lower edge of pair */
  Fixed cn; /* character coord for ideal upper edge of pair */
  Fixed f; /* device coord for ideal lower edge of pair */
  Fixed n; /* device coord for ideal upper edge of pair */
  Fixed mn; /* character coord min extent for this stem */
  Fixed mx; /* character coord max extent for this stem */
  Fixed ff; /* device pixel edge for lower edge post coloring */
  Fixed nn; /* device pixel edge for upper edge post coloring */
  Fixed w1; /* unmodified device width */
  Fixed hw2; /* desired halfwidth in character space */
  short int pathLen, index;
  struct _gcntr *clist;
  struct _gcntr *parent;
  } GlbClr, *PGlbClr;

typedef struct _gcntr { /* counter */
  struct _gcntr *next;
  Fixed w;	/* device width */
  /* the next and width fields must be first to match GCList */
  PGlbClr upper, lower;
  short int group:8;
  boolean tested:8;
  Fixed grpMn, grpMx, grpMean;
  } GlbCntr, *PGlbCntr;



