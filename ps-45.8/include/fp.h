/*
  fp.h

Copyright (c) 1986, '87, '88, '89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft, 1986
Edit History:
Scott Byer: Fri May 19 16:45:53 1989
Ivor Durham: Sat Aug 20 12:28:02 1988
Ed Taft: Thu Oct  5 15:47:06 1989
Perry Caro: Wed Nov  2 15:18:27 1988
Jim Sandman: Thu Oct 19 11:29:05 1989

End Edit History.

This interface defines a large collection of floating point, fixed point,
and various math library procedures, as well as a few in-line macros.
These are needed by the PostScript kernel. Their implementations may
be machine and/or environment dependent; some may benefit from special
hardware or assembly code.
*/

#ifndef	FP_H
#define	FP_H

#include ENVIRONMENT
#include PUBLICTYPES

/*
 * Math Library Interface
 */

/* The following procedures correspond to standard C library procedures,
   but with an "os_" prefix. They can normally be implemented as a
   veneer over the standard C library procedures, which are usually
   accessed via math.h. The purpose of renaming them is to insulate
   the PostScript kernel from details of the C library interface on
   the target machine, and also to allow modified implementations to
   be substituted without causing name conflicts.

   The main difference between these procedures and the ones in the
   C library is in their error behavior. When an error is detected,
   all the procedures below raise an exception, using the facilities
   defined in except.h; specifically, they execute one of:

     RAISE(ecRangeCheck, NIL);
     RAISE(ecUndefResult, NIL);

   where ecRangeCheck and ecUndefResult are error codes defined in
   except.h. The error code ecRangeCheck indicates that the function is
   mathematically not defined for the arguments given (for example,
   os_sqrt(x) where x is negative). The error code ecUndefResult
   indicates that the result cannot be represented as a floating point
   number.

   In contrast, C library procedures usually indicate an error either
   by storing a code in the external variable "errno" or by generating
   a signal that can be caught by a SIGFPE signal handler (some
   libraries do both, depending on the cause of the error). In the
   former case, a veneer procedure must initialize errno before calling
   the C library procedure, test it afterward, and call "RAISE" if
   an error is indicated. In the latter case, "RAISE" may be called
   from within an implementation-provided SIGFPE signal handler;
   no checking is required in the veneer procedure.

   All floating point types are explicitly declared to be double
   so as to eliminate any uncertainty about whether or not the C
   compiler performs automatic float to double promotion.
 */

extern double os_atan2(/* double num, den */);
/* Returns the angle (in radians between -pi and pi) whose tangent is
   num/den. Either num or den may be zero, but not both. The signs
   of num and den determine the quadrant in which the result will
   lie: a positive num yields a result in the positive y plane;
   a positive den yields a result in the positive x plane.
 */

extern double os_atof(/* char *str */);
/* Converts a string *str to a floating point number and returns it.
   The string consists of zero or more tabs and spaces, an optional
   sign, a series of digits optionally containing a decimal point,
   and optionally an exponent consisting of "E" or "e", an optional
   sign, and one or more digits. Processing of the string terminates
   at the first character not conforming to the above description
   (the character may but need not be null). An error results if
   no number can be recognized or if it is unrepresentable.
 */

extern double os_ceil(/* double x */);
/* Returns the least integer value greater than or equal to x. */

extern double os_cos(/* double x */);
/* Returns the cosine of x, where x represents an angle in radians. */

extern double os_exp(/* double x */);
/* Returns the exponential of x. */

extern double os_floor(/* double x */);
/* Returns the greatest integer value less than or equal to x. */

extern double os_frexp(/* double x, integer *exp */);
/* Stores an exponent in *exp and returns a value in the interval
   [0.5, 1.0) such that value * 2**exponent = x. If x is zero,
   both *exp and the returned value are set to zero.
 */

extern double os_ldexp(/* double x, integer exp */);
/* Returns the value x * 2**exp. If the result is too large to represent,
   an exception is raised; if an underflow occurs, zero is substituted. */

extern double os_log(/* double x */);
/* Returns the natural (base e) logarithm of x. */

extern double os_log10(/* double x */);
/* Returns the common (base 10) logarithm of x. */

extern double os_modf(/* double x, double *ipart */);
/* Breaks the value x into integer and fractional parts; stores the
   integer part in *ipart and returns the fractional part. Both
   results have the same sign as x.
 */

extern double os_pow(/* double base, exp */);
/* Returns base**exp, i.e., base raised to the exp power. If base is
   negative, exp must be an integer; otherwise an error is raised.
 */

extern double os_sin(/* double x */);
/* Returns the sine of x, where x represents an angle in radians. */

extern double os_sqrt(/* double x */);
/* Returns the square root of x. */


/* Inline procedures */


#define os_abs(x)	((x)<0?-(x):(x))
/* Returns the absolute value of whatever is passed in, no matter the
   type. Whatever is passed in may be evaluated twice, so it should never be
   used with an expression - those can be passed to  either os_labs for
   an integer expression or os_fabs for a float expression.
*/

#define os_labs(x)	((integer)((x)<0?-(x):(x)))
/* Similar to os_abs, but restricted to integers */

/* See also os_fabs (below) */



/*
 * Floating Point Interface
 */

/* Data Structures */

typedef union {	/* Floating point representations */
  real native;	/* whatever the native format is */
#if SWAPBITS
  struct {BitField fraction: 23, exponent: 8, sign: 1;} ieee;
    /* IEEE float, low-order byte first */
#else SWAPBITS
  struct {BitField sign: 1, exponent: 8, fraction: 23;} ieee;
    /* IEEE float, high-order byte first */
#endif SWAPBITS
  unsigned char bytes[4];
    /* raw bytes, in whatever order they appear in memory */
  } FloatRep;

/* Exported Procedures */

/* The following procedures convert floating point numbers between
   this machine's "native" representation and IEEE standard
   representations with either byte order. They all pass arguments
   and results by pointer so that C does not perform inappropriate
   conversions; in any call, the "from" and "to" pointers must be
   different. They all raise the exception ecUndefResult if the
   source value cannot be represented in the destination format
   (e.g., dynamic range exceeded, NAN, etc.)

   The conversions are done in-line when the native representation
   is the same as IEEE (perhaps with bytes reordered); otherwise they
   are done by calling a procedure. The macros and the procedures
   have the same semantics otherwise; they operate according to
   the procedure declarations given below.

   The argument designated as "real *" is a native real; it must not
   be a formal parameter of a procedure, which may be a double even
   if declared to be a real. The argument designated as "FloatRep *"
   is an IEEE floating point number. Although its representation may
   be of little interest to the caller, it must be declared as (or cast
   to) type "FloatRep *" in order to satisfy alignment requirements.
*/

#define CopySwap4(from, to) \
  (((FloatRep *) to)->bytes[0] = ((FloatRep *) from)->bytes[3], \
   ((FloatRep *) to)->bytes[1] = ((FloatRep *) from)->bytes[2], \
   ((FloatRep *) to)->bytes[2] = ((FloatRep *) from)->bytes[1], \
   ((FloatRep *) to)->bytes[3] = ((FloatRep *) from)->bytes[0] )

#if IEEEFLOAT

#if SWAPBITS
#define IEEELowToNative(from, to) *(to) = ((FloatRep *) from)->native
#define NativeToIEEELow(from, to) ((FloatRep *) to)->native = *(from)
#define IEEEHighToNative(from, to) CopySwap4((from), (to))
#define NativeToIEEEHigh(from, to) CopySwap4((from), (to))
#else SWAPBITS
#define IEEEHighToNative(from, to) *(to) = ((FloatRep *) from)->native
#define NativeToIEEEHigh(from, to) ((FloatRep *) to)->native = *(from)
#define IEEELowToNative(from, to) CopySwap4((from), (to))
#define NativeToIEEELow(from, to) CopySwap4((from), (to))
#endif SWAPBITS

#else IEEEFLOAT

extern procedure IEEEHighToNative(/* FloatRep *from, real *to */);
/* Converts from IEEE high-byte-first real to native real. */

extern procedure NativeToIEEEHigh(/* real *from, FloatRep *to */);
/* Converts from native real to IEEE high-byte-first real. */

extern procedure IEEELowToNative(/* FloatRep *from, real *to */);
/* Converts from IEEE low-byte-first real to native real. */

extern procedure NativeToIEEELow(/* real *from, FloatRep *to */);
/* Converts from native real to IEEE low-byte-first real. */

#endif IEEEFLOAT

/*
  Some compilers mis-manage the storage allocation for floating point
  constants, allocating a new location for each occurrence of the
  constant.  To minimize the storage cost a few constants are defined
  here either as constants or as references to statics.  Define
  FPCONSTANTS as zero to use the pre-allocated statics.  Do nothing to
  use the constants inline.
 */

#ifndef	FPCONSTANTS
#define	FPCONSTANTS 1
#endif	FPCONSTANTS

#if	FPCONSTANTS
#define fpZero  0.0
#define fpOne  1.0
#define fp1p3333333  1.3333333
#define fp1p5707963268  1.5707963268
#define fpTwo  2.0
#define fp3  3.0
#define fp3p1415926535  3.1415926535
#define fp4  4.0
#define fp5  5.0
#define fp6  6.0
#define fp6p2831853071  6.2831853071
#define fp8  8.0
#define fp10  10.0
#define fp50  50.0
#define fp72  72.0
#define fp90  90.0
#define fp100  100.0
#define fp180  180.0
#define fp270  270.0
#define fp360  360.0
#define fp1024  1024.0
#define fp16k  16000.0
#define fp65536  65536.0
#define fp1073741824  1073741824.0
#define  fpp001  0.001
#define  fpp015  0.015
#define  fpp03  0.03
#define  fpp05  0.05
#define  fpp1  0.1
#define  fpp11  0.11
#define  fpp2  0.2
#define  fpp25  0.25
#define  fpp3  0.3
#define  fpp3364  0.3364
#define  fpp45  0.45
#define  fpHalf  0.5
#define  fpp515  0.515
#define  fpp53  0.53
#define  fpp552  0.552
#define  fpp59  0.59
#define  fpp7  0.7
#define  fpp9  0.9
#else	FPCONSTANTS
#if (OS != os_mpw)
#define extended longreal
#endif (OS != os_mpw)
typedef struct {
  extended g_fpZero,
    g_fpOne,
    g_fp1p3333333,
    g_fp1p5707963268,
    g_fpTwo,
    g_fp3,
    g_fp3p1415926535,
    g_fp4,
    g_fp5,
    g_fp6,
    g_fp8,
    g_fp6p2831853071,
    g_fp10,
    g_fp50,
    g_fp72,
    g_fp90,
    g_fp100,
    g_fp180,
    g_fp270,
    g_fp360,
    g_fp1024,
    g_fp16k,
    g_fp65536,
    g_fp1073741824,
    g_fpp001,
    g_fpp015,
    g_fpp03,
    g_fpp05,
    g_fpp1,
    g_fpp11,
    g_fpp2,
    g_fpp25,
    g_fpp3,
    g_fpp3364,
    g_fpp45,
    g_fpHalf,
    g_fpp515,
    g_fpp53,
    g_fpp552,
    g_fpp59,
    g_fpp7,
    g_fpp9;
} DPSFPGlobalsRec;

extern DPSFPGlobalsRec *dpsfpglobals;

#define fpZero dpsfpglobals->g_fpZero
#define fpOne dpsfpglobals->g_fpOne
#define fp1p3333333 dpsfpglobals->g_fp1p3333333
#define fp1p5707963268 dpsfpglobals->g_fp1p5707963268
#define fpTwo dpsfpglobals->g_fpTwo
#define fp3 dpsfpglobals->g_fp3
#define fp3p1415926535 dpsfpglobals->g_fp3p1415926535
#define fp4 dpsfpglobals->g_fp4
#define fp5 dpsfpglobals->g_fp5
#define fp6 dpsfpglobals->g_fp6
#define fp8 dpsfpglobals->g_fp8
#define fp6p2831853071 dpsfpglobals->g_fp6p2831853071
#define fp10 dpsfpglobals->g_fp10
#define fp50 dpsfpglobals->g_fp50
#define fp72 dpsfpglobals->g_fp72
#define fp90 dpsfpglobals->g_fp90
#define fp100 dpsfpglobals->g_fp100
#define fp180 dpsfpglobals->g_fp180
#define fp270 dpsfpglobals->g_fp270
#define fp360 dpsfpglobals->g_fp360
#define fp1024 dpsfpglobals->g_fp1024
#define fp16k dpsfpglobals->g_fp16k
#define fp65536 dpsfpglobals->g_fp65536
#define fp1073741824 dpsfpglobals->g_fp1073741824
#define fpp001 dpsfpglobals->g_fpp001
#define fpp015 dpsfpglobals->g_fpp015
#define fpp03 dpsfpglobals->g_fpp03
#define fpp05 dpsfpglobals->g_fpp05
#define fpp1 dpsfpglobals->g_fpp1
#define fpp11 dpsfpglobals->g_fpp11
#define fpp2 dpsfpglobals->g_fpp2
#define fpp25 dpsfpglobals->g_fpp25
#define fpp3 dpsfpglobals->g_fpp3
#define fpp3364 dpsfpglobals->g_fpp3364
#define fpp45 dpsfpglobals->g_fpp45
#define fpHalf dpsfpglobals->g_fpHalf
#define fpp515 dpsfpglobals->g_fpp515
#define fpp53 dpsfpglobals->g_fpp53
#define fpp552 dpsfpglobals->g_fpp552
#define fpp59 dpsfpglobals->g_fpp59
#define fpp7 dpsfpglobals->g_fpp7
#define fpp9 dpsfpglobals->g_fpp9

#endif	FPCONSTANTS

/* Inline Procedures */

/* The following macros implement comparisons of real (float) values
   with zero. If IEEESOFT is true (defined in environment.h), these
   comparisons are done more efficiently than the general floating
   point compare subroutines would typically do them.
   Note: the argument r must be a simple variable (not an expression)
   of type real (float); it must not be a formal parameter
   of a procedure, which is a double even if declared to be a float.
 */

#if IEEESOFT
#define RtoILOOPHOLE(r) (*(integer *)(&(r)))
#define RealEq0(r) ((RtoILOOPHOLE(r)<<1)==0)
#define RealNe0(r) ((RtoILOOPHOLE(r)<<1)!=0)
#define RealGt0(r) (RtoILOOPHOLE(r)>0)
#define RealGe0(r) ((RtoILOOPHOLE(r)>=0)||RealEq0(r))
#define RealLt0(r) ((RtoILOOPHOLE(r)<0)&&RealNe0(r))
#define RealLe0(r) (RtoILOOPHOLE(r)<=0)
#else IEEESOFT
#define RealEq0(r) ((r)==fpZero)
#define RealNe0(r) ((r)!=fpZero)
#define RealGt0(r) ((r)>fpZero)
#define RealGe0(r) ((r)>=fpZero)
#define RealLt0(r) ((r)<fpZero)
#define RealLe0(r) ((r)<=fpZero)
#endif IEEESOFT

#define os_fabs(x) (RealLt0(x)?-(x):(x))
/* Returns the absolute value of x, which must be a simple variable
   of type real (float), as described above. */

#define RAD(a) ((a)*1.745329251994329577e-2)
/* Converts the float or double value a from degrees to radians. */

#define DEG(x) ((x)*57.29577951308232088)
/* Converts the float or double value x from radians to degrees. */

#if IEEEFLOAT
#define IsValidReal(pReal) (((FloatRep *) pReal)->ieee.exponent != 255)
#else IEEEFLOAT
extern boolean IsValidReal(/* float *pReal */);
#endif IEEEFLOAT
/* Returns true iff the float at *pReal is a valid floating point number
   (i.e., not infinity, NaN, reserved operand, etc.) */

/*
 * Fixed Point interface
 */

/* Data Structures */

/* A fixed point number consists of a sign bit, i integer bits, and f
   fraction bits; the underlying representation is simply an integer.
   The integer n represents the fixed point value n/(2**f). The current
   definition is pretty heavily dependent on an integer being 32 bits.

   There are three kinds of fixed point numbers:
	Fixed	i = 15, f = 16, range [-32768, 32768)
	Frac	i = 1, f = 30, range [-2, 2)
	UFrac	i = 2, f = 30, unsigned, range [0, 4)
   The type "Fixed", defined in the basictypes interface, is simply
   a typedef for integer and is type-equivalent to integer as far
   as the C compiler is concerned.

   Within the fp interface, the types Fixed, Frac, and UFrac are used to
   document the kinds of fixed point numbers that are the arguments
   and results of procedures. However, most clients of this interface
   declare variables to be of type Fixed, or occasionally of type integer,
   without specifying which kind of fixed point number is actually
   intended. Most Fixed numbers are of the first kind; the other two
   are provided as a convenience to certain specialized algorithms in
   the graphics machinery.

   All procedures, including the conversions from real, return a
   correctly signed "infinity" value FixedPosInf or FixedNegInf
   if an overflow occurs; they return zero if an underflow occurs.
   No overflow or underflow traps ever occur; it is the caller's
   responsibility either to detect an out-of-range result or to
   prevent it from occurring in the first place.

   All procedures compute exact intermediate results and then round
   according to some consistent rule such as IEEE "round to nearest".
 */

typedef long int /* Fixed, */ Frac, UFrac;

#define FixedPosInf MAXinteger
#define FixedNegInf MINinteger

/* Exported Procedures */

extern Fixed fixmul(/* Fixed x, y */);
/* Multiples two Fixed numbers and returns a Fixed result. */

extern Frac fracmul(/* Frac x, y */);
/* Multiples two Frac numbers and returns a Frac result. */

extern Fixed fxfrmul(/* Fixed x, Frac y */);
/* Multiplies a Fixed number with a Frac number and returns a Fixed
   result. The order of the Fixed and Frac arguments does not matter. */

extern Fixed fixdiv(/* Fixed x, y */);
/* Divides the Fixed number x by the Fixed number y and returns a
   Fixed result. */

extern Fixed tfixdiv(/* Fixed x, y */);
/* Same as fixdiv except that instead of rounding to the nearest
   representable value, it truncates toward zero. It thus guarantees
   that |q| * |y| <= |x|, where q is the returned quotient. */

extern Fixed fracratio(/* Frac x, y */);
/* Divides the Frac number x by the Frac number y and returns a
   Fixed result. */

extern Fixed ufixratio(/* Card32 x, y */);
/* Divides the unsigned integer x by the unsigned integer y and
   returns a Fixed result. */

extern Frac fixratio(/* Fixed x, y */);
/* Divides the Fixed number x by the Fixed number y and returns a
   Frac result. */

extern UFrac fracsqrt(/* UFrac x */);
/* Computes the square root of the UFrac number x and returns a
   UFrac result. */

/* The following type conversions pass their float arguments and results
   by value; the C language forces them to be of type double.
 */

extern double fixtodbl(/* Fixed x */);
/* Converts the Fixed number x to a double. */

extern Fixed dbltofix(/* double d */);
/* Converts the double d to a Fixed. */

extern double fractodbl(/* Frac x */);
/* Converts the Frac number x to a double. */

extern Frac dbltofrac(/* double d */);
/* Converts the double d to a Frac. */

/* The following type conversions pass their float arguments and results
   by pointer. This circumvents C's usual conversion of float arguments
   and results to and from double, which can have considerable
   performance consequences. Caution: do not call these procedures
   with pointers to float ("real") formal parameters, since those are
   actually of type double!
 */

extern procedure fixtopflt(/* Fixed x, float *pf */);
/* Converts the Fixed number x to a float and stores the result at *pf. */

extern Fixed pflttofix(/* float *pf */);
/* Converts the float number at *pf to a Fixed and returns it. */

extern procedure fractopflt(/* Fixed x, float *pf */);
/* Converts the Frac number x to a float and stores the result at *pf. */

extern Frac pflttofrac(/* float *pf */);
/* Converts the float number at *pf to a Frac and returns it. */


/* Inline Procedures */

#define FixInt(x) (((Fixed)(x)) << 16)
/* Converts the integer x to a Fixed and returns it. */

#define FTrunc(x) (((integer)(x))>>16)
/* Converts the Fixed number x to an integer, truncating it to the
   next lower integer value, and returns it. */

#define FCeil(x) (((integer)(x+0xFFFF))>>16)
/* Returns the smallest integer greater that or equal to the Fixed number
   x. */

#define FRound(x) ((((integer)(x))+(1<<15))>>16)
/* Converts the Fixed number x to an integer, rounding it to the
   nearest integer value, and returns it. */

#define FTruncF(x) ((Fixed)((integer)(x) & 0xFFFF0000))
#define FCeilF(x) (FTruncF((x) + 0xFFFF))
#define FRoundF(x) ((((integer)(x))+(1<<15)) & 0xFFFF0000)
/* Like above but return Fixed instead of integer. */


/*
 * Double precision integer interface
 */

/* Data structures */

typedef struct _t_Int64 {		/* Double precision integer */
#if SWAPBITS
  Card32 l; Int32 h;
#else SWAPBITS
  Int32 h; Card32 l;
#endif SWAPBITS
  } Int64, *PInt64;

/* Exported Procedures */

/* Note: for dpneg, dpadd, dpsub, and muldiv, the arguments and results
   declared as Int32 or Int64 may equally well be signed fixed point
   numbers with their binary point in an arbitrary position.
   This does not hold for dpmul or dpdiv. */

extern procedure dpneg(/* PInt64 pa, pResult */);
/* Computes the negative of the double-precision integer in *pa
   and stores the result in *pResult. Overflow is not detected.
   It is acceptable for pResult to be the same as pa. */

extern procedure dpadd(/* PInt64 pa, pb, pResult */);
/* Computes the sum of the double-precision integers in *pa and *pb
   and stores the result in *pResult. Overflow is not detected.
   It is acceptable for pResult to be the same as either pa or pb. */

extern procedure dpsub(/* PInt64 pa, pb, pResult */);
/* Computes the difference of the double-precision integers in *pa and *pb
   and stores the result in *pResult. Overflow is not detected.
   It is acceptable for pResult to be the same as either pa or pb. */

extern procedure dpmul(/* Int32 a, b, PInt64 pResult */);
/* Computes the signed product of a and b and stores the double-precision
   result in *pResult. It is not possible for an overflow to occur. */

extern Int32 dpdiv(/* PInt64 pa, Int32 b, boolean round */);
/* Divides the double-precision integer in *pa by the single-precision
   integer b and returns the quotient. If round is true, the quotient
   is rounded to the nearest integer; otherwise it is truncated
   toward zero. If an overflow occurs, MAXinteger or MINinteger is
   returned (no overflow or underflow traps ever occur). */

extern Int32 muldiv(/* Int32 a, b, c, boolean round */);
/* Computes the expression (a*b)/c, retaining full intermediate precision;
   the result is rounded or truncated to single precision only after
   the division. The descriptions of dpmul and dpdiv pertain here also. */

/*
 * Matrix Interface
 */

/*
  The following procedures manipulate a 3 x 3 transformation matrix:

	a	b	0
	c	d	0
	tx	ty	1

  where the non-constant elements are represented by a 6-element
  structure Mtx. For details, see the PostScript Language Reference
  Manual, section 4.4.

  These procedures may raise the exception ecUndefResult if an overflow
  occurs during the computation.
*/

/* Data types */
    
typedef struct _t_Mtx
    {
    float a, b, c, d, tx, ty;
    } Mtx, *PMtx;

/* Exported Procedures */

extern Cd DTfmCd( /* Cd c, PMtx m */ );
  /* (Delta-Transform) returns the result of multiplying "c" * "m" with
     its translation components zero. */

extern procedure DTfmPCd( /* Cd c, PMtx m, PCd rc */ );
  /* (Delta-Transform) in *rc returns the result of multiplying "c" * "m" with
     its translation components zero. */

extern Cd IDTfmCd( /* Cd c, PMtx m */ );
  /* (Inverse-Delta-Transform) returns the Cd c', such that
     c' * "m" with zero translation components = "c".
     This function raises the exception ecUndefResult if the
     matrix is not invertible. */

extern procedure IDTfmPCd( /* Cd c, PMtx m, PCd rc */ );
  /* (Inverse-Delta-Transform) returns the Cd *rc, such that
     *rc * "m" with zero translation components = "c".
     This function raises the exception ecUndefResult if the
     matrix is not invertible. */

extern Cd ITfmCd( /* Cd c, PMtx m */ );
  /* (Inverse-Transform) returns the Cd c', such that c' * "m" = "c".
     This function raises the exception ecUndefResult if the
     matrix is not invertible. */

extern procedure ITfmPCd( /* Cd c, PMtx m, PCd rc */ );
  /* (Inverse-Transform) returns the Cd *rc, such that *rc * "m" = "c".
     This function raises the exception ecUndefResult if the
     matrix is not invertible. */

extern procedure IdentityMtx( /* PMtx m */ );
  /* fills in "m" with an identity transformation matrix. */

extern procedure MtxCnct( /* PMtx m, n, r */ );
  /* fills in the result matrix "r" with the product of "m" * "n" */

extern boolean MtxEqAlmost( /* PMtx m1, m2 */ );
  /* returns true iff corresponding components of m1 and m2 are all
     within epsilon of each other as defined by the current device. */

extern procedure MtxInvert( /* PMtx m, im */ );
  /* fills in the result matrix "im" with the inverse "m".
     This function raises the exception ecUndefResult if the
     matrix is not invertible. */

extern procedure RtatMtx( /* Preal ang, PMtx m */ );
  /* fills in the "m" with a transformation matrix:
       cos(ang)  sin(ang)
       -sin(ang) cos(ang)
       0         0
     that performs a rotation of "ang" degrees about the origin. */

extern procedure ScalMtx( /* Preal sx, sy, PMtx m */ );
  /* fills in "m" with a transformation matrix:
       sx 0
       0  sy
       0  0
     that performs a scaling in x by "sx" and in y by "sy".  */

extern Cd TfmCd( /* Cd c, PMtx m */ );
  /* returns the Cd result of "c" * "m". */

extern procedure TfmPCd( /* Cd c, PMtx m, PCd rc */ );
  /* returns in *rc the Cd result of "c" * "m". */

extern procedure TlatMtx( /* Preal tx, ty, PMtx m */ );
  /* fills in "m" with a transformation matrix:
       0  0
       0  0
       tx ty
     that performs a translation by (tx, ty).  */

/*
 * Vector interface
 */

extern procedure VecAdd( /* Cd v1, v2,  PCd v3 */ );
  /* returns the sum v1 + v2 in *v3. */

extern procedure VecSub( /* Cd v1, v2,  PCd v3 */ );
  /* returns the difference v1 - v2 in *v3 */

extern procedure VecMul( /* Cd v, Preal r, PCd v2 */ );
  /* returns the scalar multiplication v * *r in *v2. */

#endif	FP_H
/* v004 taft Tue Dec 1 11:01:03 PST 1987 */
/* v005 durham Sun Feb 7 12:39:08 PST 1988 */
/* v006 sandman Thu Mar 10 10:07:24 PST 1988 */
/* v007 taft Tue Mar 29 18:27:28 PST 1988 */
/* v008 durham Fri May 6 16:21:56 PDT 1988 */
/* v008 durham Wed Jun 15 14:20:04 PDT 1988 */
/* v009 durham Thu Aug 11 13:01:39 PDT 1988 */
/* v010 caro Wed Nov 2 17:28:13 PST 1988 */
/* v011 bilodeau Wed Apr 26 12:54:24 PDT 1989 */
/* v012 byer Mon May 15 16:39:29 PDT 1989 */
/* v013 sandman Mon Oct 16 15:17:33 PDT 1989 */
/* v014 taft Thu Nov 23 14:59:40 PST 1989 */
