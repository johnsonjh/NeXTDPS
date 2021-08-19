/*
  environment.h

Copyright (c) 1984, '85, '86, '87, '88, 89 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Ed Taft: Sun Sep 23 13:48:17 1984
Edit History:
Ivor Durham: Thu Nov  3 15:18:22 1988
Ed Taft: Sun Dec 17 17:14:56 1989
Operator: Fri Oct 30 11:12:18 1987
End Edit History.

IMPORTANT: The definitions in this file comprise an extensible set.
The only permissible modifications are those that add new definitions
or new elements to enumerations. Modifications that would affect the
meanings of existing definitions are not allowed.

Most packages (including many package interfaces) depend on environment.h.
However, this dependence is not visible in package makefiles, so
changing environment.h does not trigger rebuilding of packages or
updating of package versions. To maintain consistency, it is crucial
that changes to environment.h be upward-compatible, as described above.
*/

#ifndef	ENVIRONMENT_H
#define	ENVIRONMENT_H

/* The following declarations are used to control various configurations
   of PostScript. The compile-time definitions ISP, OS, STAGE, and
   LANGUAGE_LEVEL (and definitions derived from them) control conditional
   compilation of PostScript; i.e., they determine what C code gets
   included and how it is compiled.

   When a package depends on one or more of these configuration
   switches, a separate library for that package must be built for
   each configuration. All packages implicitly depend on ISP and OS,
   even if they do not mention those switches explicitly, since the
   compiled code is necessarily different for each instruction set and
   development environment.

   The individual values of the ISP and OS enumerations have names
   of the form "isp_xxx" and "os_yyy". The names xxx and yyy are
   the ones selected by the ISP and OS definitions in the makefile
   for each configuration of every package and product. Furthermore,
   the configuration directories themselves are usually named by
   concatenating the ISP and os names, i.e., "xxxyyy". (Exception:
   CONTROLLER is substituted for ISP to name configuration
   directories in a few packages.)

   Runtime switches of the form vXXX are initialized from the command line
   and control the data (mainly operators) that are entered into the VM.
   This enables a development configuration of PostScript running in
   one environment to generate the initial VM for a different
   configuration and/or execution environment. (Such cross-development
   cannot be done arbitrarily; it is implemented only in specific cases.)
   These runtime switches are actually macros that refer to fields
   of a Switches structure, whose definition is given below.

   There are two kinds of switches: master configuration switches and
   derived option switches. The master switches, normally defined in
   the cc command line, are as follows.
 */

/* ***** Master Switches ***** */

/*
 * ISP (Instruction Set Processor) definitions -- these specify
 * the CPU architecture of the machine on which the configuration
 * is to execute.
 *
 * The compile-time switch ISP specifies the configuration for which
 * the code is to be compiled. The runtime switch vISP specifies the
 * configuration for which an initial VM is to be built.
 */

#define isp_vax		1
#define isp_mc68010	2	/* includes mc68000 */
#define isp_mc68020	3	/* includes mc68030 */
#define isp_i80286	4
#define isp_i80386	5
#define isp_ibm370	6
#define isp_ti34010	7
#define isp_ibmrt	8
#define isp_r2000be	9	/* MIPS, big-endian */
#define isp_r2000le	10	/* MIPS, little-endian */
#define isp_sparc	11
#define isp_ns32532	12
#define isp_xl8000	13	/* Weitek */

/*
 * OS (Operating System) definitions -- these specify the operating
 * system and runtime environment in which the configuration is
 * to execute.
 *
 * The compile-time switch OS specifies the environment for which
 * the code is to be compiled. The runtime switch vOS specifies the
 * target environment for which an initial VM is to be built, which may
 * be different from the host environment in which the building is done.
 */

#define os_ps		1	/* Adobe PostScript runtime package */
#define os_bsd		2	/* Unix -- Berkeley Software Distribution */
#define os_sun		3	/* Unix -- Sun Microsystems */
#define os_sysv		4	/* Unix -- AT&T System V */
#define os_aux		5	/* Unix -- Apple A/UX */
#define os_xenix	6	/* Unix -- MicroSoft Xenix */
#define os_vms		7	/* DEC VMS */
#define os_vaxeln	8	/* DEC VAXELN */
#define os_domain	9	/* Apollo Domain */
#define os_mpw		10	/* Apple Macintosh Programmer's Workshop */
#define os_vm370	11	/* IBM VM */
#define os_mvs370	12	/* IBM MVS */
#define os_os2		13	/* MicroSoft OS/2 */
#define os_ultrix	14	/* Unix -- DEC Ultrix */
#define os_aix		15	/* IBM Adv. Interactive eXecutive */
#define os_pharlap	16	/* Proprietary DOS extension for 386 */
#define os_mach		17	/* MACH from CMU (Unix 4.3 compatible) */
#define os_msdos	18	/* MicroSoft DOS */

/*
 * STAGE = one of (order is important). If STAGE is DEVELOP, the runtime
 * switch vSTAGE may be EXPORT to specify the target VM being built.
 */

#define DEVELOP		1
#define EXPORT		2

/*
 * LANGUAGE_LEVEL specifies the overall set of language features to be
 * included in the product. Higher levels are strict supersets of
 * lower ones. At the moment, this affects the compilation of code
 * in many packages. A future re-modularization may eliminate the need
 * for this compile-time switch.
 *
 * If STAGE is DEVELOP, the runtime switch vLANGUAGE_LEVEL specifies
 * the set of language features to be included in the target whose
 * VM is being built; this must be a subset of the language features
 * present in the host configuration.
 */

#define level_1		1	/* original red book */
#define level_2		2	/* PS level 2; new red book */
#define level_dps	3	/* Display PostScript; assumed to be
				   a strict extension of level 2 */
#ifndef LANGUAGE_LEVEL
#define LANGUAGE_LEVEL level_dps
#endif


/*
 * C_LANGUAGE is a boolean switch that controls whether C language
 * declarations in this interface are to be compiled. It is normally
 * true; setting it to false suppresses such declarations, thereby
 * making this interface suitable for inclusion in assembly language
 * or other non-C modules.
 */

#ifndef C_LANGUAGE
#define C_LANGUAGE 1
#endif C_LANGUAGE

/*
 * ANSI_C is a boolean switch that determines whether the compiler
 * being used conforms to the ANSI standard. This controls conditional
 * compilation of certain constructs that must be written differently
 * according to whether or not ANSI C is being used (for example, the
 * CAT macro defined below).
 */

#ifndef ANSI_C
#ifdef __STDC__
#define ANSI_C 1
#else
#define ANSI_C 0
#endif
#endif ANSI_C

/*
 * CAT(a,b) produces a single lexical token consisting of a and b
 * concatenated together. For example, CAT(Post,Script) produces
 * the identifier PostScript.
 */

#ifndef CAT
#if ANSI_C
#define CAT(a,b) a##b
#else ANSI_C
#define IDENT(x) x
#define CAT(a,b) IDENT(a)b
#endif ANSI_C
#endif CAT

/*
 * Define ANSI C storage class keywords as no-ops in non-ANSI environments
 */

#if !ANSI_C
#define const
#define volatile
#endif !ANSI_C


/* ***** Derived environmental switches *****

  These switches describe features of the environment that must be
  known at compile time. They are all a function of ISP and OS;
  note that only certain ISP-OS combinations are defined.

  [Eventually, these switches will be moved into separate <ISP><OS>.h
  files, one for each defined ISP-OS combination.]
 
  In a few cases, for a given compile-time switch XXX, there is a
  runtime switch vXXX. This enables a host system to build an initial
  VM for a target system with a different environment. These runtime
  switches are a function of vISP and vOS.

  MC68K means running on a MC680x0 CPU. This controls the presence
  of in-line 680x0 assembly code in certain C modules.

  IEEEFLOAT is true if the representation of type "float" is the IEEE
  standard 32-bit floating point format, with byte order as defined
  by SWAPBITS (below). IEEEFLOAT is false if some other representation
  is used (or, heaven forbid, IEEE representation with some inconsistent
  byte order). This determines the conditions under which conversions
  are required when manipulating external binary representations.

  IEEESOFT means that the representation of type "float" is the IEEE
  standard 32-bit floating point format, AND that floating point
  operations are typically performed in software rather than hardware.
  This enables efficient in-line manipulation of floating point values
  in certain situations (see the FP interface).

  SWAPBITS is true on a CPU whose native order is "little-endian", i.e.,
  the low-order byte of a multiple-byte unit (word, longword) appears
  at the lowest address in memory. SWAPBITS is false on a "big-endian"
  CPU, where the high-order byte comes first. This affects the layout
  of structures and determines whether or not conversions are required
  when manipulating external binary representations.

  The runtime switch vSWAPBITS specifies the bit order of the machine
  for which the VM is being built. If vSWAPBITS is different from
  SWAPBITS, the endianness of the VM and font cache must be reversed
  as they are written out. This is a complex operation that is
  implemented for only certain combinations of development and
  target architectures.

  IMPORTANT: the representation of raster data (in frame buffer, font
  cache, etc.) is determined by the display controller, which is not
  necessarily consistent with the CPU bit order. The bit order of
  raster data is entirely hidden inside the device implementation.

  UNSIGNEDCHARS means that the type "char", as defined by the compiler,
  is unsigned (otherwise it is signed).

  MINALIGN and PREFERREDALIGN specify the alignment of structures
  (mainly in VM) containing word and longword fields; all such
  structures are located at addresses that are a multiple of the
  specified values.

  MINALIGN is the minimum alignment required by the machine architecture
  in order to avoid hardware errors. Its sole purpose is to control
  compilation of code for accessing data known to be misaligned.

  PREFERREDALIGN is the alignment that results in the most efficient
  word and longword accesses; it should usually be the memory bus
  width of the machine in bytes. The runtime switch vPREFERREDALIGN
  specifies the preferred alignment in the target machine for which
  the VM is being built.
 */

#if ISP==isp_vax && (OS==os_bsd || OS==os_ultrix || OS==os_vms || OS==os_vaxeln)
#define MC68K 0
#define IEEEFLOAT 0
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_mc68010 && OS==os_ps
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 2
#define PREFERREDALIGN 2
#endif

#if ISP==isp_mc68020 && (OS==os_sun || OS==os_ps)
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_mc68020 && OS==os_mach
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_mc68020 && OS==os_mpw
#define MC68K 1
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_i80286 /* && OS==? */
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 2
#endif

#if ISP==isp_i80386 && (OS==os_aix || OS==os_pharlap || OS==os_xenix)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_ibm370 && (OS==os_vm370 || OS==os_mvs370)
#define MC68K 0
#define IEEEFLOAT 0
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 1
#define MINALIGN 2
#define PREFERREDALIGN 4
#endif

#if ISP==isp_ti34010  /* && OS==? */
#define MC68K 0
#define IEEEFLOAT 0
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_ibmrt && OS==os_aix
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 0
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#endif

#if ISP==isp_r2000be && (OS==os_sysv || OS==os_ps)
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#endif

#if ISP==isp_r2000le && OS==os_ultrix
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 1
#define MINALIGN 4
#define PREFERREDALIGN 4
#endif

#if ISP==isp_sparc && OS==os_sun
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 0
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#endif

#if ISP==isp_ns32532  /* && OS==? */
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 0
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 1
#define PREFERREDALIGN 4
#endif

#if ISP==isp_xl8000 && OS==os_ps
#define MC68K 0
#define IEEEFLOAT 1
#define IEEESOFT 1
#define SWAPBITS 1
#define UNSIGNEDCHARS 0
#define MINALIGN 4
#define PREFERREDALIGN 4
#endif

#ifndef IEEEFLOAT
ConfigurationError("Unsupported ISP-OS combination");
#endif


/* ***** Other configuration switches *****
 *
 * These switches are more-or-less independent of machine architecture;
 * their default values are derived from STAGE. They affect the compiled
 * code in many packages.
 */

/*
 * "register" declarations are disabled in certain configurations because
 * they interfere with correct operation of the debugger or because
 * they provoke compiler bugs.
 */

#ifndef	NOREGISTER
#define NOREGISTER (STAGE==DEVELOP)
#endif	NOREGISTER

#if	NOREGISTER
#define register
#endif	NOREGISTER

/*
 * VMINIT means the ability exists to initialize a VM from scratch and
 * to build a VM for a target configuration different from the host
 * configuration doing the building. !VMINIT means that the initial
 * VM must be pre-built (obtained from a file or bound into the code
 * itself).
 */

#ifndef VMINIT
#define VMINIT (STAGE==DEVELOP)
#endif

/*
 * MAKEVM means we are building a VM for a target configuration whose
 * STAGE is EXPORT. It is false at compile time in EXPORT configurations.
 * MAKEVM is used to suppress the definitions of operators that are
 * always included in DEVELOP configurations and also in the EXPORT
 * configurations of some but not all products. If "op" is such an
 * operator, it should be registered by:
 *   if (! MAKEVM) RgstExplicit("op", PSOp);
 * Also, "op" must be registered by an opdef while building the EXPORT VM.
 */

#ifndef MAKEVM
#define MAKEVM (STAGE==DEVELOP && vSTAGE==EXPORT)
#endif


/* ***** Runtime Switches ***** */

#if C_LANGUAGE

/*
 * The vXXX runtime switches are actually references into this data
 * structure, which is saved as part of the VM.
 */

typedef struct {
  unsigned char
    _ISP,
    _OS,
    _STAGE,
    _LANGUAGE_LEVEL,
    _SWAPBITS,
    _PREFERREDALIGN,
    unused[2];	/* for nice alignment */
} Switches;

extern Switches switches;

/* Change this whenever Switches is changed to invalidate saved VMs: */
#define SWITCHESVERSION 3

#define vISP (switches._ISP)
#define vOS (switches._OS)
#define vSTAGE (switches._STAGE)
#define vLANGUAGE_LEVEL (switches._LANGUAGE_LEVEL)
#define vSWAPBITS (switches._SWAPBITS)
#define vPREFERREDALIGN (switches._PREFERREDALIGN)

#endif C_LANGUAGE

#endif	ENVIRONMENT_H
/* v005 durham Tue Apr 19 09:43:26 PDT 1988 */
/* v007 bilodeau Wed Apr 26 12:35:58 PDT 1989 */
/* v008 taft Thu Nov 23 14:55:48 PST 1989 */
