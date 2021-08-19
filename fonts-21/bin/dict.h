/*
 * dict.h
 */

#ifndef _dict
#define _dict

typedef struct _DictElement
	{
	char *key;
	char *value;
	struct _DictElement *next;
	} DictElement;

typedef struct
	{
	int length;
	int size;
	DictElement *elements[1];
	} Dict;

Dict *newDict(/* int */);
void disposeDict(/* Dict * */);

void defDict(/* Dict *, char *, void * */);
char *getDict(/* Dict *, char * */);

int sizeDict(/* Dict * */);

void enumerateDict(/* Dict *, void (*)() */);

#endif
