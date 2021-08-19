#import "FontString.h"
#import <stdlib.h>
#import <string.h>

@implementation FontString

+ new:(const char *)aString
{
    self = [super new];
    s = strcpy((char *)malloc(strlen(aString)+1), aString);
    return self;
}

- free
{
    free(s);
    return [super free];
}

- (const char *)value;
{
    return s;
}

- (int)length
{
    return strlen(s);
}

@end


