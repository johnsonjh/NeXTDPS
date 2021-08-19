/*
	buldafmdir.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

/*
 * The format of the font directory (.fontdirectory) is:
 *
 * (int)sizeof(int)		- can be used to determine byte sex as well
 * (char)???			- filler so that sizeof(int) + filler == 4
 * (int)count of fonts in the file
 *
 * (int)family
 * (int)face
 * (char)weight
 * (char)flags
 * (char)flags2
 * (char)flags3
 * (int)size (if screen font)
 * (int)afmfile
 *
 * (int)family
 * (int)face
 * (char)weight
 * (char)flags
 * (char)flags2
 * (char)flags3
 * (int)size (if screen font)
 * (int)afmfile
 *
 * ...
 *
 * string1\0string2\0string3\0 ...
 *
 * The ints above are indexes into the string table at the end.
 */

#define STALE_LOCK_TIMEOUT 3600
#define STANDARD_ENCODING_SCHEME "AdobeStandardEncoding"

