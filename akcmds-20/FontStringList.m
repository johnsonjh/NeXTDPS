#import "FontStringList.h"
#import "FontString.h"
#import <objc/List.h>
#import <objc/HashTable.h>
#import <string.h>

@implementation FontStringList

+ new
{
    self = [super new];
    list = [List new];
    hashtable = [HashTable newKeyDesc:"*" valueDesc:"i"];
    return self;
}

- free
{
    [hashtable free];
    [list freeObjects];
    [list free];
    return [super free];
}
    
- (int)add:(const char *)string
{
    id fs;
    int current;

    if ([hashtable isKey:string]) {
	return (int)[hashtable valueForKey:string];
    } else {
	fs = [FontString new:string];
	[list addObject:fs];
	[hashtable insertKey:[fs value] value:(void *)offset];
	current = offset;
	offset += strlen(string)+1;
	return current;
    }
}

- dump:(NXStream *)s
{
    id fs;
    int i, count;

    count = [list count];
    for (i = 0; i < count; i++) {
	fs = [list objectAt:i];
	NXWrite(s, [fs value], [fs length]+1);
    }

    return self;
}

@end

