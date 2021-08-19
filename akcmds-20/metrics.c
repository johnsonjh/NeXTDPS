#import "metrics.h"
#import <appkit/nextstd.h>
#import <streams/streams.h>

extern const char *programName;

static const char *isPrefix(const char *prefix, const char *string)
{
    if (!string || !prefix) return NULL;

    while (*string && *prefix == *string) {
	prefix++;
	string++;
    }

    return *prefix ? NULL : string;
}

static FontMetrics *newFontMetrics()
{
    FontMetrics *fm;

    NX_MALLOC(fm, FontMetrics, 1);
    fm->name = NULL;
    fm->familyName = NULL;
    fm->weight = NULL;
    fm->fullName = NULL;
    fm->encodingScheme = NULL;
    fm->italicAngle = 361.0;
    fm->weightValue = 0;
    fm->faceName = NULL;

    return fm;
}

#ifdef TEMP_DEBUGGING
static void printFontMetrics(FontMetrics *fm)
{
    printf("Name: %s\nFull Name: %s\nFamily Name: %s\nWeight: %s\n",
	fm->name, fm->fullName, fm->familyName, fm->weight);
    printf("Encoding: %s\nItalic angle: %f\n",
	fm->encodingScheme, fm->italicAngle);
}
#endif

static char *copyUpToNewLine(const char *source)
{
    int length;
    char *retval, *newline;

    newline = strchr(source, '\n');
    if (newline) {
	length = newline - source;
    } else {
	length = strlen(source);
    }
    NX_MALLOC(retval, char, length+1);
    strncpy(retval, source, length);
    retval[length] = '\0';

    return retval;
}

#define dup_afm_error(metric) fprintf(stderr, "%s: duplicate %s afm entry in %s (using first found)\n", programName, metric, file);
#define missing_afm_error(metric) fprintf(stderr, "%s: %s afm entry not found in %s\n", programName, metric, file);

FontMetrics *readMetrics(const char *file)
{
    int count = 0;
    const char *value;
    int done = 0;
    char *data;
    int length, maxlen;
    FontMetrics *fm = NULL;
    NXStream *s;

    s = NXMapFile(file, NX_READONLY);
    if (s) {
	NXGetMemoryBuffer(s, &data, &length, &maxlen);
	if (data) {
	    fm = newFontMetrics();
	    while (!done) {
		if (*data == 'F') {
		    switch (*(data+1)) {
			case 'o': value = isPrefix("FontName ", data);
				  if (value) {
					if (!fm->name) {
					    count++;
					    fm->name = copyUpToNewLine(value);
					} else {
					    dup_afm_error("FontName");
					}
				  }
				  break;
			case 'u': value = isPrefix("FullName ", data);
				  if (value) {
					if (!fm->fullName) {
					    count++;
					    fm->fullName = copyUpToNewLine(value);
					} else {
					    dup_afm_error("FullName");
					}
				  }
				  break;
			case 'a': value = isPrefix("FamilyName ", data);
				  if (value) {
					if (!fm->familyName) {
					    count++;
					    fm->familyName = copyUpToNewLine(value);
					} else {
					    dup_afm_error("FamilyName");
					}
				  }
				  break;
		    }
		} else if (*data == 'W') {
		    value = isPrefix("Weight ", data);
		    if (value) {
			if (!fm->weight) {
			    count++;
			    fm->weight = copyUpToNewLine(value);
			} else {
			    dup_afm_error("Weight");
			}
		    }
		} else if (*data == 'E') {
		    value = isPrefix("EncodingScheme ", data);
		    if (value) {
			if (!fm->encodingScheme) {
			    count++;
			    fm->encodingScheme = copyUpToNewLine(value);
			} else {
			    dup_afm_error("EncodingScheme");
			}
		    }
		} else if (*data == 'I') {
		    value = isPrefix("ItalicAngle ", data);
		    if (value) {
			if (fm->italicAngle > 360.0) {
			    count++;
			    fm->italicAngle = atof(value);
			} else {
			    dup_afm_error("ItalicAngle");
			}
		    }
		} else if (*data == 'f') {
		    value = isPrefix("faceName ", data);
		    if (value) {
			if (!fm->faceName) {
			    fm->faceName = copyUpToNewLine(value);
			} else {
			    dup_afm_error("faceName");
			}
		    }
		} else if (*data == 'w') {
		    value = isPrefix("weightValue ", data);
		    if (value) {
			if (!fm->weightValue) {
			    const char *weightString = copyUpToNewLine(value);
			    fm->weightValue = atoi(weightString);
			    NX_FREE(weightString);
			} else {
			    dup_afm_error("weightValue");
			}
		    }
		}
		data = strchr(data, '\n');
		if (data) {
		    data += 1;
		} else {
		    done = 1;
		}
		done = done || (count == 6);
	    }
	}
	NXClose(s);
    }

    if (!fm) return NULL;

    if (!fm->encodingScheme) {
	missing_afm_error("EncodingScheme");
	fm->encodingScheme = copyUpToNewLine("AdobeStandardEncoding\n");
	count++;
    }
    if (fm->italicAngle > 360.0) {
	missing_afm_error("ItalicAngle");
	fm->italicAngle = 0.0;
	count++;
    }

    if (count == 6) {
	return fm;
    } else {
	if (!fm->fullName)
	    missing_afm_error("FullName");
	if (!fm->name)
	    missing_afm_error("FontName");
	if (!fm->familyName)
	    missing_afm_error("FamilyName");
	if (!fm->weight)
	    missing_afm_error("Weight");
	fprintf(stderr, "%s: invalid metrics in %s (ignoring)\n",
	    programName, file);
	freeFontMetrics(fm);
	return NULL;
    }
}

void freeFontMetrics(FontMetrics *fm)
{
    if (fm) {
	NX_FREE(fm->name);
	NX_FREE(fm->familyName);
	NX_FREE(fm->fullName);
	NX_FREE(fm->weight);
	NX_FREE(fm->encodingScheme);
	NX_FREE(fm->faceName);
	NX_FREE(fm);
    }
}

