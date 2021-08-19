/*
 * prebuild.c
 *
 */

#define RESOLUTION 75.0
#define DEPTH 1

#include <stdio.h>
#include <string.h>
#include "prebuiltformat.h"
#include "dict.h"

#define DEBUG 0

int strcmpcompare(/* char **, char ** */);
char *strnew(/* char * */);
char *strlin(/* char * */);

#define MAXinteger ((long int)0x7FFFFFFF)
#define MINinteger ((long int)0x80000000)
#define FixedPosInf MAXinteger
#define FixedNegInf MINinteger
#define fixedScale 65536.0  /* i=15, f=16, range [-32768, 32768) */
typedef long int Fixed; /*  16 bits of integer, 16 bits of fraction */

Fixed pflttofix(pf)
  float *pf;
  {
  float f = *pf;
  if (f >= FixedPosInf / fixedScale) return FixedPosInf;
  if (f <= FixedNegInf / fixedScale) return FixedNegInf;
  return (Fixed) (f * fixedScale);
  }

void fadjust(/* FILE * */);

void error(/* char *, char * */);
int toBinary(/* char */);

void initPrebuiltFile(/* PrebuiltFile *, char *, long, char *, Dict *, PrebuiltWidth *, char *, long, PrebuiltMatrix *, int, PrebuiltMask * */);
void writePrebuiltFile(/* PrebuiltFile *, char * */);

Dict *getEncoding(/* char * */);
int getDepth(/* char * */);
char *getNames(/* Dict *, long * */);
PrebuiltWidth *getWidths(/* char *, Dict * */);
PrebuiltMatrix getMatrix(/* char *, int */);
void getMasks(/* char *, PrebuiltMask *, Dict *, int */);

#define MASKBYTES(w, h, d) (((w * d + 7) / 8) * h)

char expandNibble[16] =
  {
  0 * 255 / 15,
  1 * 255 / 15,
  2 * 255 / 15,
  3 * 255 / 15,
  4 * 255 / 15,
  5 * 255 / 15,
  6 * 255 / 15,
  7 * 255 / 15,
  8 * 255 / 15,
  9 * 255 / 15,
  10 * 255 / 15,
  11 * 255 / 15,
  12 * 255 / 15,
  13 * 255 / 15,
  14 * 255 / 15,
  15 * 255 / 15
  };

/*
  BigEndian returns 1 if this program is running on a
  big endian machine, 0 if on a little endian machine.
  */
int BigEndian()
{
  int  i;
  
  i = 1;
  return((*((unsigned char *)(&i))) == 0);
} /* BigEndian */

#undef PREBUILT_SUFFIX
#define PREBUILT_SUFFIX (BigEndian() ? ".bepf" : ".lepf")

#ifdef THINK_C
void _main(/* int, char ** */);
void _main(argc, argv)
#else
void main(/* int, char ** */);
void main(argc, argv)
#endif
  int argc;
  char **argv;
  {
  PrebuiltFile prebuiltFile;
  Dict *encoding;
  int depth;
  PrebuiltWidth *widths;
  PrebuiltMatrix *matrices;
  char *names;
  long lengthNames;
  PrebuiltMask *masks;
  int i;
  long identifier,fontType;
  
  if (argc < 5)
    {
    printf("usage: prebuild fontName fontType fontIdentifier characterSetName fontFile ...\n");
    return;
    }
  
  encoding = getEncoding(argv[5]);
  
  depth = getDepth(argv[5]);

  widths = getWidths(argv[5], encoding);

  names = getNames(encoding, &lengthNames);

  matrices = (PrebuiltMatrix *) malloc(sizeof(PrebuiltMatrix) * (argc - 5));
  for (i = 0; i < argc - 5; i++)
    matrices[i] = getMatrix(argv[i + 5], depth);

  masks = (PrebuiltMask *) malloc(sizeof(PrebuiltMask) * sizeDict(encoding) * (argc - 5));
  for (i = 0; i < argc - 5; i++)
    {
    printf("file: %s\n", argv[i + 5]);
    getMasks(argv[i + 5], &masks[sizeDict(encoding) * i], encoding, depth);
    }

  sscanf(argv[2], "%ld", &fontType);
  sscanf(argv[3], "%ld", &identifier);
  initPrebuiltFile(&prebuiltFile, argv[1], (fontType<<24)|identifier, argv[4], encoding, widths, names, lengthNames, matrices, argc - 5, masks);

  {
      char outputFileName[256];
      
      strcpy(outputFileName,argv[1]);
      strcat(outputFileName,PREBUILT_SUFFIX);
      writePrebuiltFile(&prebuiltFile,outputFileName);
  }
} /* main */

void initPrebuiltFile(prebuiltFile, fontName, identifier, characterSetName, encoding, widths, names, lengthNames, matrices, numberMatrix, masks)
  PrebuiltFile *prebuiltFile;
  char *fontName;
  long identifier;
  char *characterSetName;
  Dict *encoding;
  PrebuiltWidth *widths;
  char *names;
  long lengthNames;
  PrebuiltMatrix *matrices;
  int numberMatrix;
  PrebuiltMask *masks;
  {
  prebuiltFile->version = PREBUILTFILEVERSION;
  prebuiltFile->byteOrder = (BigEndian() ? 1 : 0);
  prebuiltFile->vertWidths = 0;
  prebuiltFile->vertMetrics = 0;
  prebuiltFile->identifier = identifier;
  prebuiltFile->fontName = (long) strnew(fontName);
  prebuiltFile->characterSetName = (long) strnew(characterSetName);
  prebuiltFile->numberChar = sizeDict(encoding);
  prebuiltFile->widths = (long) widths;
  prebuiltFile->names = (long) names;
  prebuiltFile->lengthNames = lengthNames;
  prebuiltFile->numberMatrix = numberMatrix;
  prebuiltFile->matrices = (long) matrices;
  prebuiltFile->masks = (long) masks;

  printf("font: %s\n", fontName);
  printf("identifier: %ld\n", identifier);
  printf("character set: %s\n", characterSetName);
  printf("characters: %d\n", prebuiltFile->numberChar);
  printf("matrices: %d\n", prebuiltFile->numberMatrix);
  }

void writePrebuiltFile(prebuiltFile, outputFile)
PrebuiltFile *prebuiltFile;
char *outputFile;
{
    FILE *output;
    long offset,masksOffset;
    int i;
    PrebuiltMatrix *matrices;
    
    /* Write placeholder version of PrebuiltFile */
    output = fopen(outputFile, "w");
    fwrite((char *) prebuiltFile, sizeof(PrebuiltFile), 1, output);
    
    /* Write out font name */
    fadjust(output);
    offset = ftell(output);
    fwrite((char *) prebuiltFile->fontName, strlen((char *) prebuiltFile->fontName) + 1, 1, output);
    prebuiltFile->fontName = offset;
#if DEBUG
    printf("font name offset %d\n", offset);
#endif
    
    /* Write out character set name */
    fadjust(output);
    offset = ftell(output);
    fwrite((char *) prebuiltFile->characterSetName, strlen((char *) prebuiltFile->characterSetName) + 1, 1, output);
    prebuiltFile->characterSetName = offset;
#if DEBUG
    printf("char set name offset %d\n", offset);
#endif
    
    /* Write out scalable character widths */
    fadjust(output);
    offset = ftell(output);
    fwrite((char *) prebuiltFile->widths, sizeof(PrebuiltWidth), prebuiltFile->numberChar, output);
    prebuiltFile->widths = offset;
#if DEBUG
    printf("char widths offset %d\n", offset);
#endif
    
    /* Write out matrices for all sizes in file */
    fadjust(output);
    offset = ftell(output);
    matrices = (PrebuiltMatrix *) prebuiltFile->matrices;
    fwrite((char *) prebuiltFile->matrices, sizeof(PrebuiltMatrix), prebuiltFile->numberMatrix, output);
    prebuiltFile->matrices = offset;
#if DEBUG
    printf("matrices offset %d\n", offset);
#endif
    
    /* Write out placeholder versions of masks (file offsets are still
       pointers at this point, so they'll have to be fixed) */
    fadjust(output);
    offset = ftell(output);
    fwrite((char *) prebuiltFile->masks, sizeof(PrebuiltMask), 
        prebuiltFile->numberChar * prebuiltFile->numberMatrix, output);
    masksOffset = offset;
#if DEBUG
    printf("mask offset %d\n", offset);
#endif
    
    /* Write out the actual bitmap data */
    fadjust(output);
    prebuiltFile->lengthMaskData = -ftell(output);
#if DEBUG
    printf("bitmap data start offset %d\n", -prebuiltFile->lengthMaskData);
#endif
    for (i = 0; i < prebuiltFile->numberChar * prebuiltFile->numberMatrix; i++)
      {
      long offset;
      PrebuiltMask *prebuiltMask;
      
      prebuiltMask = &((PrebuiltMask *) prebuiltFile->masks)[i];
    
      fadjust(output);
      offset = ftell(output);
      fwrite((char *) prebuiltMask->maskData, 1, 
        MASKBYTES(prebuiltMask->width, prebuiltMask->height, 
        matrices[i / prebuiltFile->numberChar].depth), output);
      prebuiltMask->maskData = offset;
      }
    prebuiltFile->lengthMaskData += ftell(output);
    fadjust(output);
    offset = ftell(output);
#if DEBUG
    printf("bitmap data end offset %d\n", offset);
#endif
    
    /* Write out final PrebuiltMasks (with correct offsets) */
    fseek(output, masksOffset, 0);
    fwrite((char *) prebuiltFile->masks, sizeof(PrebuiltMask), 
        prebuiltFile->numberChar * prebuiltFile->numberMatrix, output);
    prebuiltFile->masks = masksOffset;
    
    /* Write out names from encoding */
    fseek(output, offset, 0);
    fwrite((char *)prebuiltFile->names,
          1,(int)prebuiltFile->lengthNames,output);
    prebuiltFile->names = offset;
#if DEBUG
    printf("names from encoding offset %d\n", offset);
    printf("end of data %d\n", ftell(output));
#endif
   
    /* Re-seek to beginning of file and write out updated PrebuiltFile */
    fseek(output, 0L, 0);
    fwrite((char *) prebuiltFile, sizeof(PrebuiltFile), 1, output);
    
    fclose(output);
}

Dict *getEncoding(font)
  char *font;
  {
  FILE *input;
  Dict *dict;
  char s[256];
  char n[256];
  char *table[256];
  int numberChar;
  int i;
  
  input = fopen(font, "r");
  for (numberChar = 0; fgets(s, sizeof s, input);)
    {
    if (sscanf(s, "STARTCHAR %s", n) == 1)
      table[numberChar++] = strnew(strlin(n));
    }
  fclose(input);
  
  qsort((char *) &table[0], numberChar, sizeof(char *), strcmpcompare);
  
  for (dict = newDict(256), i = 0; i < numberChar; i++)
    {
#if 0
    printf("\t\"%s,\"\t/* %d */\n", table[i], i);
#endif
    defDict(dict, table[i], (void *) (i + 1));
    }

  return dict;
  }

int getDepth(font)
  char *font;
  {
  FILE *input;
  char s[256];
  int depth;

  input = fopen(font, "r");
  for (depth = 1; fgets(s, sizeof s, input);)
    {
    if (sscanf(s, "DEPTH %d", &depth) == 1)
      break;
    }
  fclose(input);

#if DEPTH
  if (depth > 1)
    depth = 8;
#endif

  return depth;
  }

PrebuiltWidth *getWidths(font, encoding)
  char *font;
  Dict *encoding;
  {
  FILE *input;
  PrebuiltWidth *widths;
  char s[256];
  char n[256];
  float x;
  float y;
  int code;

  input = fopen(font, "r");
  for (widths = (PrebuiltWidth *) malloc(sizeof(PrebuiltWidth) * 256); fgets(s, sizeof s, input);)
    {
    if (sscanf(s, "STARTCHAR %s", n) == 1)
      strlin(n);
    else if (sscanf(s, "SWIDTH %f %f", &x, &y) == 2)
      {
      code = ((int) getDict(encoding, n)) - 1;
      if (code >= 0)
        {
        x /= 1000.0;
        widths[code].hx = pflttofix(&x);
        y /= 1000.0;
        widths[code].hy = pflttofix(&y);
        }
      else
        error("getWidths: can't find name", n);
      }
    }
  fclose(input);
  
  return widths;
  }

int *enumerateLengthNamesArray;
char *enumerateNamesArray;
void enumerateLengthNames(/* char *, void * */);
void enumerateLengthNames(key, value)
  char *key;
  char *value;
  {
  int i = (int) value;
  enumerateLengthNamesArray[i] = strlen(key) + 1;
  }

void enumerateNames(/* char *, void * */);
void enumerateNames(key, value)
  char *key;
  char *value;
  {
  int i = (int) value;
  strcpy(&enumerateNamesArray[enumerateLengthNamesArray[i - 1]], key);
  }

char *getNames(encoding, length)
  Dict *encoding;
  long *length;
  {
  int lengthNames[257];
  char *names;
  int i;
  
  for (i = 0; i <= sizeDict(encoding); i++)
    lengthNames[i] = 0;

  enumerateLengthNamesArray = lengthNames;
  enumerateDict(encoding, enumerateLengthNames);

  for (i = 1; i <= sizeDict(encoding); i++)
    lengthNames[i] += lengthNames[i - 1];
    
  *length = lengthNames[sizeDict(encoding)];
  names = (char *)malloc(*length);
  
  enumerateLengthNamesArray = lengthNames;
  enumerateNamesArray = names;
  enumerateDict(encoding, enumerateNames);

  return names;
  }

PrebuiltMatrix getMatrix(font, depth)
  char *font;
  int depth;
  {
  FILE *input;
  PrebuiltMatrix matrix;
  char s[256];
  float size;
  float x;
  float y;
  float r;
  
  input = fopen(font, "r");
  for (; fgets(s, sizeof s, input);)
    {
    if (sscanf(s, "SIZE %f %f %f", &size, &x, &y) == 3)
      {
      r = (float) size * RESOLUTION / (float) x;
      matrix.a = pflttofix(&r);
      matrix.b = 0;
      matrix.c = 0;
      r = (float) size * RESOLUTION / (float) y;
      matrix.d = pflttofix(&r);
      matrix.tx = 0;
      matrix.ty = 0;
      matrix.depth = depth;
      break;
      }
    }
  fclose(input);

  return matrix;
  }

void getMasks(font, masks, encoding, depth)
  char *font;
  PrebuiltMask *masks;
  Dict *encoding;
  int depth;
  {
  FILE *input;
  char s[256];
  char n[256];
  int bitmap;
  int code;
  int width;
  int height;
  int ox;
  int oy;
  int wx;
  int wy;
  char *dst;
  char *src;
  int i;

  input = fopen(font, "r");
  for (bitmap = -1; fgets(s, sizeof s, input);)
    {
    if (sscanf(s, "STARTCHAR %s", n) == 1)
      strlin(n);
    else if (sscanf(s, "BBX %d %d %d %d", &width, &height, &ox, &oy) == 4)
      ;
    else if (sscanf(s, "DWIDTH %d %d", &wx, &wy) == 2)
      ;
    else if (strncmp(s, "BITMAP", 6) == 0)
      {
      bitmap = 0;
      code = ((int) getDict(encoding, n)) - 1;
      if (code >= 0)
        {
        if (width < 0 || width > 255)
          error("getMasks: width rangecheck", "");
        else
          masks[code].width = width;
        if (height < 0 || height > 255)
          error("getMasks: height rangecheck", "");
        else
          masks[code].height = height;
        ox = -ox;
        if (ox < -128 || ox > 127)
          error("getMasks: x offset rangecheck", "");
        else
          masks[code].maskOffset.hx = ox;
        oy = height + oy;
        if (oy < -128 || oy > 127)
          error("getMasks: y offset rangecheck", "");
        else
          masks[code].maskOffset.hy = oy;
        if (wx < -128 || wx > 127)
          error("getMasks: x width rangecheck", "");
        else
          masks[code].maskWidth.hx = wx;
        if (wy < -128 || wy > 127)
          error("getMasks: y width rangecheck", "");
        else
          masks[code].maskWidth.hy = wy;
        masks[code].maskData = (long) malloc(MASKBYTES(width, height, depth));
        }
      else
        error("getMasks: can't find name", n);
      }
    else if (strncmp(s, "ENDCHAR", 7) == 0)
      bitmap = -1;
    else if (bitmap != -1 && strlen(s) > 1)
      {
      dst = (char *) masks[code].maskData + MASKBYTES(width, bitmap, depth);
      src = s;
#if DEPTH
      if (depth > 1)
        for (i = 0; i < MASKBYTES(width, 1, depth); i++, dst++, src++)
          *dst = expandNibble[toBinary(*src)];
      else
#endif
        for (i = 0; i < MASKBYTES(width, 1, depth); i++, dst++, src += 2)
          *dst = (toBinary(*src) << 4) + toBinary(*(src + 1));
      bitmap++;
      }
    }
  fclose(input);
  }

char *strnew(a)
  char *a;
  {
  return strcpy((char *)malloc(strlen(a) + 1), a);
  }

char *strlin(a)
  char *a;
  {
  int l;

  l = strlen(a);
  if (l && a[l - 1] == '\r')
    a[l - 1] = '\0';
  return a;
  }

void error(a, b)
  char *a;
  char *b;
  {
  printf("%s %s\n", a, b);
  exit(-1);
  }

int toBinary(a)
  char a;
  {
  return a <= '9' ? a - '0' : a <= 'F' ? a - 'A' + 10 : a - 'a' + 10;
  }

int strcmpcompare(a, b)
  char **a;
  char **b;
  {
  return strcmp(*a, *b);
  }

void fadjust(file)
  FILE *file;
  {
  long offset;
  char null[4];
  
  null[0] = null[1] = null[2] = null[3] = '\0';
  offset = ftell(file);
  if (offset %= 4)
    fwrite(null, 1, 4 - (int) offset, file);
  }
