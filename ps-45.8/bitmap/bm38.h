/* bm38.h --- class interface file */

#define AMASK		0x000000FF
#define RGBMASK		0xFFFFFF00
#define RBMASK		0xFF00FF00
#define GAMASK		0x00FF00FF

#define RGB_WHITE	0xFFFFFF00
#define RGB_LTGRAY	0xAAAAAA00

#define SRCBITMAP   0
#define SRCCONSTANT 1

typedef struct {
    int op;
    int width;			/* In pixels */
    int height;			/* In scanlines */
    int srcType;		/* Type of source: SRCCONSTANT or SRCBITMAP */
    int *srcPtr;		/* Pointer to source bits */
    int srcInc;			/* Source Pixel to pixel address increment */
    int srcRowBytes;		/* Optimized source rowBytes */
    int srcValue;		/* Virtual source constant */
    int srcOnscreen;		/* boolean */
    int *dstPtr;		/* Pointer to destination bits */
    int dstInc;			/* Dst Pixel to pixel address increment */
    int dstRowBytes;		/* Optimized destination RowBytes */
    int dstOnscreen;		/* boolean */
    int delta;			/* Dissolve delta */
    unsigned int mask;		/* General mask */
    unsigned int rmask;		/* Mask against read data */
    unsigned int ltor;		/* Left to Right? */
    unsigned int ttob;		/* Top to Bottom? */
} RectOp;
