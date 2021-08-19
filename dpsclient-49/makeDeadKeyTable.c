
/*
    makeDeadKeyTable.c

    This is a one file program that takes as input a list of dead key character combinations, and outputs a C file with this info in a data structure.
    
    Usage:	    makeDeadKeyTable [<infile>]
    
    The input file should entirely consist of lines of the form:

	<char1> <char2> <result char>

    Each line of this format means that if <char1> is typed followed by <char 2>, then <result char> will be placed in the event queue as if it had been typed.  Each char can either be a number or a single character between single quotes.  Lines starting with '#' and blank lines are ignored.
    
    The resulting file will declare two tables.  The first, DiacritcalInfoOffsets, will be an array of 256 unsigned chars.  Each of these values will be an offset into the second table, one per character.  If a character can be the <char1> of a dead key combo, then it will have a non-zero value in this table.  The second table, DiacritcalInfo, holds a series of 0 terminated strings.  Each string consists of pairs of chars.  Each pair is a <char2> and its corresponding <char result>.  Offsets in the first table lead to a string in the second table.

    Copyright (c) 1988 NeXT, Inc. as an unpublished work.
    All rights reserved.
*/

#include <stdio.h>	
#include <stdlib.h>	
#include <string.h>	
#include <ctype.h>

static void error(const char *msg);
static void readData(FILE *inFile);
static void writeFile(FILE *outFile);
static unsigned char *skipWS(unsigned char *s);
static unsigned char *getChar(unsigned char *s, unsigned char *charRead);
static int charCompare(const void *d1, const void *d2);

unsigned char Table[256][3];		/* info for all dead keys */
int TableSize = 0;


main(int argc, char *argv[])
{
    FILE *inFile;
    FILE *outFile;

    if (argc == 1)
	inFile = stdin;
    else if (argc == 2) {
	if(!(inFile = fopen(argv[1], "r"))) {
	    perror(argv[0]);
	    exit(-1);
	}
    } else {
	fprintf(stderr, "Usage: %s [<infile>]\n", argv[0]);
	exit(-1);
    }
    readData(inFile);
    qsort(Table, TableSize, 3, &charCompare);
    writeFile(stdout);
}


#define MAX_BUF	256

static void readData(FILE *inFile)
{
    unsigned char buffer[MAX_BUF];
    unsigned char *s;
    unsigned char currChar;
    int i;

    while (fgets((char *)buffer, MAX_BUF, inFile)) {
	if (*buffer == '#')
	    continue;
	s = skipWS(buffer);
	if (!s || !*s)
	    continue;
	for (i = 0; i < 3; i++) {
	    s = skipWS(s);
	    if (!s || !*s)
		error("Bad file format");
	    s = getChar(s, &(Table[TableSize][i]));
	    if (!s || !*s)
		error("Bad file format");
	}
	TableSize++;
    }
}


static unsigned char *skipWS(unsigned char *s)
{
    while (*s && isspace(*s))
	s++;
    return s;
}


static unsigned char *getChar(unsigned char *s, unsigned char *charRead)
{
    int val;

    if (*s == '\'') {
	*charRead = *(s+1);
	return s+3;
    } else if (isdigit(*s)) {
	val = atoi((char *)s);
	if (val >= 0 && val <= 255) {
	    *charRead = val;
	    while (isdigit(*s))
		s++;
	    return s;
	} else
	    error("Character value out of range");
    } else
	error("Bad format for character token");
}


static void writeFile(FILE *of)
{
    int c;
    int tabIndex;
    int currOffset;
    int lastChar;

    fprintf(of, "\n/* This file generated by makeDeadKeyTable */\n" );
    fprintf(of, "/* Copyright 1988, NeXT, Inc. */\n\n\n" );

    fprintf(of, "static const unsigned char DiacritcalInfoOffsets[256] = {" );
    tabIndex = 0;
    currOffset = 1;	  /* first entry is wasted so 0 means no combos */
    for (c = 0; c < 256; c++) {
	if (!(c % 16))
	    fprintf(of, "\n\t\t");
	if (tabIndex < TableSize && c == Table[tabIndex][0]) {
	  /* +1 since first entry is wasted so 0 means no combos */
	    fprintf(of, "%d, ", currOffset);
	    while (tabIndex < TableSize && c == Table[tabIndex][0]) {
		tabIndex++;
		currOffset += 2;	/* skip second and result char */
	    }
	    currOffset++;		/* skip null terminator */
	} else
	    fprintf(of, "0, ");
    }
    fprintf(of, "\n\t};\n\n");

    fprintf(of, "static const unsigned char DiacritcalInfo[] = { " );
    lastChar = -1;
    for (tabIndex = 0; tabIndex < TableSize; tabIndex++) {
	if (lastChar != Table[tabIndex][0]) {
	    fprintf(of, "0,\n\t\t");
	    lastChar = Table[tabIndex][0];
	}
	fprintf(of, "%d, %d, ", Table[tabIndex][1], Table[tabIndex][2]);
    }
    fprintf(of, "0\n\t};\n\n");
}


static int charCompare(const void *d1, const void *d2)
{
    char *c1 = (char *)d1;
    char *c2 = (char *)d2;

    if (c1[0] - c2[0])
	return c1[0] - c2[0];
    else
	return c1[1] - c2[1];
}


static void error(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(-1);
}

