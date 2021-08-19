#import "FontFamily.h"
#import "FontFace.h"
#import <appkit/FontManager.h>
#import <objc/List.h>

@implementation FontFamily

+ newFamily:(int)index
{
    self = [super new];
    family = index;
    faces = [List new];
    return self;
}

- free
{
    [faces freeObjects];
    [faces free];
    return [super free];
}

- (unsigned int)count
{
    return [faces count];
}

- (BOOL)matches:(int)index
{
    return(index == family);
}

- addFace:(int)name weight:(int)weight
    flags:(int)flags size:(int)size afm:(int)afm
{
    id face;
    
    face = [FontFace new:name weight:weight flags:flags size:size afm:afm];
    [faces addObject:face];

    return self;
}

- findFaceWithWeight:(int)weight andFlags:(int)flags
{
    id face;
    int i, count;

    count = [faces count];
    for (i = 0; i < count; i++) {
	face = [faces objectAt:i];
	if ([face weight] == weight && [face flags] == flags) {
	    return face;
	}
    }

    return nil;
}

/*
  Bold weight is 9 then 8 then 7.
  Plain weight is 5 then 6 then 4.
*/

- dump:(NXStream *)s
{
    int flags;
    id face, better;
    int i, count, weight;

    count = [faces count];
    for (i = 0; i < count; i++) {
	face = [faces objectAt:i];
	weight = [face weight];
	flags = [face flags] & (~(NX_BOLD|NX_UNBOLD));
	if (weight == 5) {
	    [face addFlag:NX_UNBOLD];
	} else if (weight == 9) {
	    [face addFlag:NX_BOLD];
	} else if (weight == 6 || weight == 4) {
	    better = [self findFaceWithWeight:5 andFlags:flags];
	    if (!better && weight == 4) {
		better = [self findFaceWithWeight:6 andFlags:flags];
	    }
	    if (!better) {
		[face addFlag:NX_UNBOLD];
	    }
	} else if (weight == 8 || weight == 7) {
	    better = [self findFaceWithWeight:9 andFlags:flags];
	    if (!better && weight == 7) {
		better = [self findFaceWithWeight:8 andFlags:flags];
	    }
	    if (!better) {
		[face addFlag:NX_BOLD];
	    }
	}
    }

    for (i = 0; i < count; i++) {
	face = [faces objectAt:i];
	NXWrite(s, &family, sizeof(int));
	[face dump:s];
    }

    return self;
}

@end


