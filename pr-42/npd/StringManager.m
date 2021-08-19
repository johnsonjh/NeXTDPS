/*
 * StringManager.m	- Implementation of the StringManager class.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 */

/*
 * Include files.
 */

#import "StringManager.h"

#import <stddef.h>

/*
 * Constants
 */
static char	*defaultDefault = "Indescribable Error";



@implementation StringManager : HashTable


/*
 * Factory methods
 */

+ new
{
    id	newSelf;			/* The new StringManager */

    /* Instantiate ourself as HashTable for with strings as keys and vals */
    newSelf = [super newKeyDesc:"*" valueDesc:"*"];

    /* Set up a default default string */
    [newSelf setDefaultString:defaultDefault];

    return (newSelf);
}

- loadFromArray:(string_entry_t)strings
{
    string_entry_t	sep;

    /* Scan through the array and add the entries to the HashTable */
    for (sep = strings; sep->se_key != NULL; sep++) {
	[self insertKey:(const void *)sep->se_key
    		 value:(void *)sep->se_val];
    }

    return (self);
}

- loadFromMachO:(const char *)sectionName
{
    [self doesNotRecognize:_cmd];
    return (self);
}
    

/*
 * Instance methods
 */

- (char *)getDefaultString
{

    return (defaultString);
}

- setDefaultString:(char *)newDefault
{

    defaultString = newDefault;
    return (self);
}

- (char *)stringFor:(const char *)key
{
    void	*val;

    if ((val = [self valueForKey:(const void *)key]) == nil) {
	return (defaultString);
    }
    return ((char *)val);
}

@end

