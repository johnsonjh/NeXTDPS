/*
 * StringManager.h	- Interface file to the string manager object.
 *
 * Copyright (c) 1990 by NeXT, Inc.  All rights reserved.
 *
 * The StringManager class allows you to centralize error messages and
 * other strings that your program emits so that they can easily be
 * changed to other languages.
 */

#import <objc/HashTable.h>

/*
 * Type definitions.
 */
typedef struct string_entry {
    const char	*se_key;
    char	*se_val;
} *string_entry_t;

/*
 * StringManager interface definition.
 */

@interface	StringManager : HashTable
    /*
     * A StringManager is a specialized HashTable that hashes character
     * strings to character strings.
     */
{
    char	*defaultString;		/* The default string */
}

/* Factory methods */
+ new;
/*
 * "new" creates a new, empty StringManager.  All it will ever return
 * is the default string.
 */

- loadFromArray:(string_entry_t)strings;
/*
 * "loadFromArray:" loads the strings from an array of string_entry
 * structures.  The final structure in the array should have a key of
 * NULL.
 */

- loadFromMachO:(const char *)sectionName;
/*
 * "loadFromMachO:" loads the strings from the MachO section
 * "sectionName" in the segment "__STRINGS".
 *
 * NOT YET IMPLEMENTED.  ONCE IT IS WE SHOULD DESCRIBE THE FORMAT OF
 * THE SECTION HERE.
 */

- (const char *)getDefaultString;
/*
 * "getDefaultString" returns a pointer to the default string.
 */

- setDefaultString:(const char *)newDefault;
/*
 * "setDefaultString" sets the default string to be "newDefault".  This
 * string is returned whenever stringFor: fails to match the key given
 * it.
 */

- (const char *)stringFor:(const char *)key;
/*
 * "stringFor:" retrieves the string for the given key.  If there is no
 * such string in the StringManager, it will return the defaultString.
 */

@end

