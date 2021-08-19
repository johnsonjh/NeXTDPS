typedef struct {
	unsigned char *tiffdata;
	unsigned char *jpegdata;
	int	w,h,spp,linenum;
	unsigned char *tiffdataalloced;
	} Globs;

extern Globs NXJPegGlobs;

/* 
 *  Offsets we want to know about in bitstream.
 */
#define QUANT0 	0
#define QUANT1 	1
#define HUFFDC0 2
#define HUFFAC0 3
#define HUFFDC1 4
#define HUFFAC1 5
#define DATA	6	
