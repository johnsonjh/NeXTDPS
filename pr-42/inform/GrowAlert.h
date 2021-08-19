
#import <objc/Object.h>

@interface GrowAlert : Object
{
  id text;
  id window;
  id button1;
  id button2;
}

+ new;

- window;
- text;

- setWindow: anObject;
- setText: anObject;

- cancel:sender;
- ok:sender;

@end

