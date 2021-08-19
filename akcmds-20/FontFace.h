#import <objc/Object.h>
#import <streams/streams.h>

@interface FontFace : Object
{
    int name;
    int weight;
    int flags;
    int size;
    int afm;
}

+ new:(int)name weight:(int)weight
    flags:(int)f size:(int)size afm:(int)afm;
- (int)weight;
- (int)flags;
- addFlag:(int)flag;
- dump:(NXStream *)s;

@end

