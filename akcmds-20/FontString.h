#import <objc/Object.h>

@interface FontString : Object
{
    char *s;
    int offset;
}

+ new:(const char *)aString;
- free;
- (const char *)value;
- (int)length;

@end


