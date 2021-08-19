/*
  STbuild.c

Copyright (c) 1984, 1986, 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Chuck Geschke: January 24, 1983
Edit History:
Chuck Geschke: Wed Jul 11 16:00:06 1984
Ed Taft: Tue Jun 14 16:19:32 1988
End Edit History.

This program, in conjunction with grammar.h, generates the tables used
by the PostScript scanner. The shell script new_grammar reads the list
of classes, states, and actions at the beginning of grammar.h; it
regenerates the various C declarations in grammar.h and in STbuild.c.
It then compiles and executes STbuild.c, which generates the new tables.
It then inserts those tables into scanner.c.
*/

#include <stdio.h>
#include "grammar.h"

/* The following arrays are regenerated automatically by the
   new_grammar script; they should not be edited manually.
 */

/* BEGIN_STRINGS */

static char *nameClass[] = {
 "eos", "crt", "lff", "dlm", "com", "qot", "lpr", "rpr", "lab", "rab", "lbr", "rbr", "lbk", "rbk", "nme", "sgn", "dot", "nbs", "oct", "dec", "exp", "escbf", "escnrt", "hex", "ltr", "oth", "btokhi", "btoklo", "btokn", "bfn", "btokhdr", "bund", "nClasses",  };

static char *nameState[] = {
 "start", "comnt", "msign", "ipart", "point", "fpart", "expon", "esign", "epart", "bbase", "bpart", "ident", "slash", "ldent", "edent", "litstr", "escchr", "escoct1", "escoct2", "hexstr", "nStates",  };

static char *nameAction[] = {
 "discard", "_discard", "disceol", "cdisceol", "_begnum", "_begxnum", "_begname", "_begxtext", "appname", "_appname", "apphex", "appint", "_appint", "appfrac", "_appfrac", "_begbnum", "appbnum", "_begexp", "appstr", "_appstr", "_strnest", "_appeol", "_appesc", "_esceol", "_begoct", "_appoct", "_uxname", "_xname", "_ulname", "_lname", "_uename", "_ename", "_strg", "_hstrg", "_uinum", "_inum", "_ubnum", "_bnum", "_urnum", "_rnum", "axbeg", "axend", "adbeg", "adend", "_empty", "_errstr", "_errini", "_errhex", "abtokhi", "abtoklo", "abtokn", "abfn", "abtokhdr", "nActions",  };

static char *nameBTAction[] = {
 "ba_objSeq", "ba_int32", "ba_int16", "ba_int8", "ba_fixed", "ba_realIEEE", "ba_realNative", "ba_bool", "ba_sstr", "ba_lstr", "ba_systemName", "ba_userName", "ba_numArray",  };
/* END_STRINGS */

typedef struct{
	State state; Action action} ScanTabItem, *STIPtr;

#define SET(s,a) {sti.state = s; sti.action = a; goto ret;}

#define NEWTOKEN \
  NEWTOKENxnme case nme:
    /* Set of classes that begin a new token, thereby terminating the
       current one (except for strings and comments). This does not
       include white space characters. In all cases, the character must
       be put back so that it is read again as part of the new token.
     */

#define NEWTOKENxnme \
  case com: case lpr: case rpr: case lab: case rab: case lbr: case rbr: \
  case lbk: case rbk: case btokhi: case btoklo: case btokn: case bfn: \
  case btokhdr: case bund:
    /* same as NEWTOKEN, but excluding nme */

int errors;

ScanTabItem InitScanTab(state,class)
	State state; Class class;
{
  ScanTabItem sti;
  switch (state)
    {
    case start:
      switch (class)
        {
	case eos: SET(start, _empty);	/* no more tokens*/
	case crt:
	case lff:
	case dlm: SET(start, discard);	/* ignore */
	case com: SET(comnt, _discard);	/* begin comment */
	case lpr: SET(litstr, _begxtext);/* begin string */
	case rpr: SET(start, _errini);	/* extra right paren */
	case lab: SET(hexstr, _begxtext);/* begin hex string */
	case rab: SET(start, _errini);	/* extra right angle bracket */
	case lbk: SET(start, adbeg);	/* begin (data) array */
	case rbk: SET(start, adend);	/* end (data) array */
	case lbr: SET(start, axbeg);	/* begin (executable) array */
	case rbr: SET(start, axend);	/* end (executable) array */
	case nme: SET(slash, _begxtext);/* begin literal or imm eval name */
	case oct:
	case dec: SET(ipart, _begnum);	/* begin integer part of number */
	case sgn: SET(msign, _begxnum);	/* begin signed number */
	case dot: SET(point, _begxnum);	/* begin fraction part of number */
	case qot:
	case nbs:
	case exp:
	case hex:
	case escbf:
	case escnrt:
	case ltr:
	case oth: SET(ident, _begname);	/* begin executable name */
	case btokhi: SET(start, abtokhi); /* process binary token */
	case btoklo: SET(start, abtoklo);
	case btokn: SET(start, abtokn);
	case bfn: SET(start, abfn);
	case btokhdr: SET(start, abtokhdr);
	case bund: SET(start, _errini);
	default: goto bug;
	}

    case comnt:
      switch (class)
        {
	case eos: SET(start, _empty);	/* no more tokens */
	case crt:
	case lff: SET(start, _discard);	/* end of comment */
	default: SET(comnt, discard);	/* continue comment */
	}

    case msign:
      switch (class)
        {
	case eos: SET(start, _xname);	/* finish as name */
	case crt: SET(msign, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _xname);	/* finish as name */
	NEWTOKEN  SET(start, _uxname);	/* back up and finish as name */
	case oct:
	case dec: SET(ipart, _appint);	/* begin integer part */
	case dot: SET(point, _appname); /* begin fractional part */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case point:
      switch (class)
        {
	case eos: SET(start, _xname);	/* finish as name */
	case crt: SET(point, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _xname);	/* finish as name */
	NEWTOKEN  SET(start, _uxname);	/* back up and finish as name */
	case oct:
	case dec: SET(fpart, _appfrac);	/* append fraction digit */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case ipart:
      switch (class)
        {
	case eos: SET(start, _inum);	/* finish as integer */
	case crt: SET(ipart, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _inum);	/* finish as integer */
	NEWTOKEN  SET(start, _uinum);	/* back up and finish as integer */
	case oct:
	case dec: SET(ipart, appint);	/* append integer digit */
	case dot: SET(fpart, _appname);	/* fraction follows */
	case nbs: SET(bbase, _appname);	/* num #, mantissa follows */
	case exp: SET(expon, _begexp);	/* exponent follows */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case bbase:
      switch (class)
        {
	case eos: SET(start, _xname);	/* finish as name */
	case crt: SET(bbase, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _xname);	/* finish as name */
	NEWTOKEN  SET(start, _uxname);	/* back up and finish as name */
	case oct:
	case dec:
	case exp:
	case hex:
	case escbf:
	case escnrt:
	case ltr: SET(bpart, _begbnum);	/* begin mantissa of based number */
	  /* _begbnum conditional state switch: if base not in [2, 36]
	     or digit >= base then SET(ident, _appname) */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case bpart:
      switch (class)
        {
	case eos: SET(start, _bnum);	/* finish as integer */
	case crt: SET(bpart, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _bnum);	/* finish as integer */
	NEWTOKEN  SET(start, _ubnum);	/* back up and finish as integer */
	case oct:
	case dec:
	case exp:
	case hex:
	case escbf:
	case escnrt:
	case ltr: SET(bpart, appbnum);	/* append to based number mantissa */
	  /* appbnum conditional state switch: if digit >= base then
	     SET(ident, _appname) */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case fpart:
      switch (class)
        {
	case eos: SET(start, _rnum);	/* finish as real */
	case crt: SET(fpart, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _rnum);	/* finish as real */
	NEWTOKEN  SET(start, _urnum);	/* back up and finish as real */
	case oct:
	case dec: SET(fpart, appfrac);	/* append fraction digit */
	case exp: SET(expon, _begexp);	/* exponent follows */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    /* Note: the actions for the exponent part do not perform the
       conversion; they just accumulate the text for later conversion.
     */

    case expon:
      switch (class)
        {
	case eos: SET(start, _xname);	/* finish as name */
	case crt: SET(expon, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _xname);	/* finish as name */
	NEWTOKEN  SET(start, _uxname);	/* back up and finish as name */
	case sgn: SET(esign, _appname);	/* get sign of exponent */
	case oct:
	case dec: SET(epart, _appname);	/* begin exponent part */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case esign:
      switch (class)
        {
	case eos: SET(start, _xname);	/* finish as name */
	case crt: SET(esign, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _xname);	/* finish as name */
	NEWTOKEN  SET(start, _uxname);	/* back up and finish as name */
	case oct:
	case dec: SET(epart, _appname);	/* begin exponent part */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case epart:
      switch (class)
        {
	case eos: SET(start, _rnum);	/* finish as real */
	case crt: SET(epart, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _rnum);	/* finish as real */
	NEWTOKEN  SET(start, _urnum);	/* back up and finish as real */
	case oct:
	case dec: SET(epart, appname);	/* append exponent digit */
	default: SET(ident, _appname);	/* not a number, continue as name */
	}

    case ident:
      switch (class)
        {
	case eos: SET(start, _xname);	/* finish as name */
	case crt: SET(ident, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _xname);	/* finish as name */
	NEWTOKEN  SET(start, _uxname);	/* back up and finish as name */
	default: SET(ident, appname);	/* continue name */
	}

    case slash:
      switch (class)
        {
	case eos: SET(start, _lname);	/* finish as literal name */
	case crt: SET(slash, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _lname);	/* finish as literal name */
	NEWTOKENxnme  SET(start, _ulname); /* back up, finish as lit name */
	case nme: SET(edent, _discard);	/* begin imm eval name */
	default: SET(ldent, _appname);	/* begin literal name */
	}

    case ldent:
      switch (class)
        {
	case eos: SET(start, _lname);	/* finish as literal name */
	case crt: SET(ldent, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _lname);	/* finish as literal name */
	NEWTOKEN  SET(start, _ulname);	/* back up, finish as lit name */
	default: SET(ldent, appname);	/* continue literal name */
	}

    case edent:
      switch (class)
        {
	case eos: SET(start, _ename);	/* finish as imm eval name */
	case crt: SET(edent, cdisceol);	/* discard CR, scan LF */
	case lff:
	case dlm: SET(start, _ename);	/* finish as imm eval name */
	NEWTOKEN  SET(start, _uename);	/* back up, finish as imm eval name */
	default: SET(edent, appname);	/* continue imm eval name */
	}

    case litstr:
      switch (class)
        {
        case eos: SET(start, _errstr);	/* syntax error */
	case crt: SET(litstr, disceol);	/* discard CR, scan LF */
	case lff: SET(litstr, _appeol);	/* append EOL to string */
	case qot: SET(escchr, _discard);/* scan escape sequence */
	case lpr: SET(litstr, _strnest);/* increment paren nesting level */
	case rpr: SET(start, _strg);	/* finish string, ignore char */
	  /* _strg conditional state switch: if nested paren then
	     SET(litstr, _appstr) */
	default: SET(litstr, appstr);	/* append to string */
        }

    case escchr:
      switch (class)
        {
        case eos: SET(start, _errstr);	/* syntax error */
	case crt:
	case lff: SET(litstr, _esceol);	/* discard "\ eol" sequence */
	case escbf:
	case escnrt: SET(litstr, _appesc);/* append mapped escape */
	case oct: SET(escoct1, _begoct);/* begin octal char code */
	default: SET(litstr, _appstr);	/* append quoted char literally */
        }

    case escoct1:
      switch (class)
        {
        case eos: SET(start, _errstr);	/* syntax error */
	case crt: SET(escoct1, disceol);/* discard CR, scan LF */
	case lff: SET(litstr, _appeol);	/* append EOL to string */
	case qot: SET(escchr, _discard);/* scan escape sequence */
	case lpr: SET(litstr, _strnest);/* increment paren nesting level */
	case rpr: SET(start, _strg);	/* finish string, ignore char */
	  /* _strg conditional state switch: if nested paren then
	     SET(litstr, _appstr) */
	case oct: SET(escoct2, _appoct);/* accumulate octal char code */
	default: SET(litstr, _appstr);	/* append to string */
        }

    case escoct2:
      switch (class)
        {
        case eos: SET(start, _errstr);	/* syntax error */
	case crt: SET(escoct2, disceol);/* discard CR, scan LF */
	case lff: SET(litstr, _appeol);	/* append EOL to string */
	case qot: SET(escchr, _discard);/* scan escape sequence */
	case lpr: SET(litstr, _strnest);/* increment paren nesting level */
	case rpr: SET(start, _strg);	/* finish string, ignore char */
	  /* _strg conditional state switch: if nested paren then
	     SET(litstr, _appstr) */
	case oct: SET(litstr, _appoct);	/* accumulate octal char code */
	default: SET(litstr, _appstr);	/* append to string */
        }

    case hexstr:
      switch (class)
        {
        case eos: SET(start,_errstr);	/* syntax error */
	case crt:
	case lff:
	case dlm: SET(hexstr, discard);	/* ignore white space */
	case oct:
	case dec:
	case hex:
	case escbf:
	case exp: SET(hexstr, apphex);	/* append hex digit */
	case rab: SET(start, _hstrg);	/* finish string, ignore char */
	default: SET(start, _errhex);	/* syntax error */
        }

    default: goto bug; /*unknown state or class*/
  }
  ret: return sti;
  bug:
    fprintf(stderr, "No entry for state %s, class %s\n",
	   nameState[state], nameClass[class]);
    errors++;
    SET(start, _discard);
}  /* end-of InitScanTab */



PrintElem(index, str)
  int index; char *str;
{
if (index == 0) printf("%s", str);
else if ((index % 5) == 0) printf(",\n  %s", str);
else printf(", %s", str);
}

main()
{
  State state; Class class; ScanTabItem sti;
  char buf[25];
  for (state = start; state < nStates; state++)
    {
    printf("private readonly StateRec %sState = {\n {", nameState[state]);
    for (class = 0; class < nClasses; class++)
      {
      sti = InitScanTab(state, class);
      PrintElem(class, nameAction[sti.action]);
      }
    printf("},\n {");
    for (class = 0; class < nClasses; class++)
      {
      sti = InitScanTab(state, class);
      PrintElem(class, nameState[sti.state]);
      if (state != sti.state && nameAction[sti.action][0] != '_')
        {
	fprintf(stderr, "State %s, class %s specifies no-switch action %s but state %s\n",
	  nameState[state], nameClass[class], nameAction[sti.action],
	  nameState[sti.state]);
	errors++;
	}
      }
    printf("}};\n\n");
    }

  printf("\nprivate readonly PStateRec stateArray[] = {\n  ");
  for (state = 0; state < nStates; state++)
    {
    sprintf(buf, "&%sState", nameState[state]);
    PrintElem(state, buf);
    }
  printf("};\n");

  exit(errors);
}
