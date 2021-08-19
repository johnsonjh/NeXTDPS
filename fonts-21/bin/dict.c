/*
 * dict.h
 */

#include "dict.h"

#if THINK_C
char *mlalloc(/* long */);
#define malloc(size) mlalloc((long) (size))
#else
char *malloc(/* int */);
#endif
void free(/* char * */);

int hashDict(/* Dict *, char * */);
int strcmp(/* char *, char * */);

Dict *newDict(length)
  int length;
  {
  Dict *dict;
  int hash;
  
  dict = (Dict *) malloc(sizeof(Dict) + length * sizeof(DictElement *));
  dict->length = length;
  dict->size = 0;
  for (hash = 0; hash < dict->length; hash++)
    dict->elements[hash] = 0L;
  
  return dict;
  }

void disposeDict(dict)
  Dict *dict;
  {
  free((char *) dict);
  }

void defDict(dict, key, value)
  Dict *dict;
  char *key;
  char *value;
  {
  int hash;
  DictElement *dictElement;
  
  hash = hashDict(dict, key);
  for (dictElement = dict->elements[hash]; dictElement && strcmp(dictElement->key, key); dictElement = dictElement->next)
    ;
  if (!dictElement)
    {
    dictElement = (DictElement *) malloc(sizeof(DictElement));
    dictElement->key = key;
    dictElement->next = dict->elements[hash];
    dict->elements[hash] = dictElement;
    dict->size++;
    }
  dictElement->value = value;
  }

char *getDict(dict, key)
  Dict *dict;
  char *key;
  {
  int hash;
  DictElement *dictElement;
  
  hash = hashDict(dict, key);
  for (dictElement = dict->elements[hash]; dictElement && strcmp(dictElement->key, key); dictElement = dictElement->next)
    ;
  return dictElement ? dictElement->value : 0L;
  }

void enumerateDict(dict, proc)
  Dict *dict;
  void (*proc)();
  {
  int hash;
  DictElement *dictElement;
  
  for (hash = 0; hash < dict->length; hash++)
    {
    for (dictElement = dict->elements[hash]; dictElement; dictElement = dictElement->next)
      (*proc)(dictElement->key, dictElement->value);
    }
  }

int sizeDict(dict)
  Dict *dict;
  {
  return dict->size;
  }

static int hashDict(dict, key)
  Dict *dict;
  char *key;
  {
  long hash;
  
  for (hash = 0; *key; )
    hash += *key++;
  return hash % dict->length;
  }
