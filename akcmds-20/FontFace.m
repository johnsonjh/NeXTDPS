#import "FontFace.h"
#import <appkit/FontManager.h>

@implementation FontFace

+ new:(int)n weight:(int)w
    flags:(int)f size:(int)s afm:(int)a
{
    self = [super new];
    name = n;
    weight = w;
    flags = f;
    size = s;
    afm = a;
    return self;
}

- (int)weight
{
    return weight;
}

- (int)flags
{
    return(flags & (~(NX_BOLD|NX_UNBOLD)));
}

- addFlag:(int)flag
{
    flags |= flag;
    return self;
}

- dump:(NXStream *)s
{
    char weightChar = weight, flags1, flags2, flags3;

    flags1 = flags & 0xff;
    flags2 = (flags & 0xff00) >> 8;
    flags3 = (flags & 0xff0000) >> 16;
    NXWrite(s, &name, sizeof(int));
    NXWrite(s, &weightChar, 1);
    NXWrite(s, &flags1, 1);
    NXWrite(s, &flags2, 1);
    NXWrite(s, &flags3, 1);
    NXWrite(s, &size, sizeof(int));
    NXWrite(s, &afm, sizeof(int));

    return self;
}

@end


