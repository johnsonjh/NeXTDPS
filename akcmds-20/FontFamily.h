#import <objc/Object.h>
#import <streams/streams.h>

@interface FontFamily : Object
{
    int family;
    id faces;
}

+ newFamily:(int)index;
- free;
- (unsigned int)count;
- (BOOL)matches:(int)index;
- addFace:(int)name weight:(int)weight
    flags:(int)flags size:(int)size afm:(int)afm;
- findFaceWithWeight:(int)weight andFlags:(int)flags;
- dump:(NXStream *)s;

@end

