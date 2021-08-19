#import <objc/Object.h>
#import <streams/streams.h>

@interface FontStringList : Object
{
    id list;
    id hashtable;
    int offset;
}

+ new;
- free;
- (int)add:(const char *)string;
- dump:(NXStream *)s;

@end


