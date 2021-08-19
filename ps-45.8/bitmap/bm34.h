/* bm34.h --- class interface file */

#define AMASK		0x000F
#define RGBMASK		0xFFF0
#define RBMASK		0xF0F0
#define GAMASK		0x0F0F

#define RGB_WHITE	0xFFF0
#define RGB_LTGRAY	0xAAA0

#define SRCBITMAP   0
#define SRCCONSTANT 1

typedef unsigned short pixel_t;

typedef struct {
    int op;
    int width;			/* In pixels */
    int height;			/* In scanlines */
    int srcType;		/* Type of source: SRCCONSTANT or SRCBITMAP */
    pixel_t *srcPtr;		/* Pointer to source bits */
    int srcInc;			/* Source Pixel to pixel address increment */
    int srcRowBytes;		/* Optimized source rowBytes */
    int srcValue;		/* Virtual source constant */
    int srcOnscreen;		/* boolean */
    pixel_t *dstPtr;		/* Pointer to destination bits */
    int dstInc;			/* Dst Pixel to pixel address increment */
    int dstRowBytes;		/* Optimized destination RowBytes */
    int dstOnscreen;		/* boolean */
    int delta;			/* Dissolve delta */
    unsigned int mask;		/* General mask */
    unsigned int rmask;		/* Mask against read data */
    unsigned int ltor;		/* Left to Right? */
    unsigned int ttob;		/* Top to Bottom? */
} RectOp;
