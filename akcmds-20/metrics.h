typedef struct {
    float italicAngle;
    char *encodingScheme;
    char *name;
    char *fullName;
    char *familyName;
    char *weight;
    char *faceName;
    int weightValue;
} FontMetrics;

extern void freeFontMetrics(FontMetrics *fm);
extern FontMetrics *readMetrics(const char *file);

