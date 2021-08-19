/*

This is valuable proprietary information and trade secrets of Creative Circuits
Corporation and is not to be disclosed or released without the prior written
permission of an officer of the company.

Copyright (c) 1989 by Creative Circuits Corporation.



This is the configuration file, to distinguish the differences between MAC, and PC 
*/

#ifndef applec
/*#define MAC 1   */
#endif


#define UNIX 1  
#define TRUE 1
#define FALSE 0

typedef	int Boolean;



#ifdef MSDOS
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <fcntl.h>
#include <math.h>
#include <sys\types.h>
#include <sys\stat.h>
#endif

#ifdef MAC
#include <stdio.h>
#include <unix.h>	
#include <fcntl.h>
/*#include <math.h>*/
#include <QuickDraw.h>
#endif

#ifdef applec
#include <values.h>
#include <types.h>
#include <Resources.h>
#include <QuickDraw.h>
#include <QuickDraw32Bit.h>
#include <fonts.h>
#include <events.h>
#include <windows.h>
#include <menus.h>
#include <textedit.h>
#include <dialogs.h>
#include <Controls.h>
#include <desk.h>
#include <toolutils.h>
#include <memory.h>
#include <Lists.h>
#include <SegLoad.h>
#include <Files.h>
#include <Packages.h>
#include <OSEvents.h>
#include <OSUtils.h>
#include <DiskInit.h>
#include <Traps.h>
#include <String.h>
#include <Strings.h>
#include <stdio.h>
#include <math.h>
#include <StdLib.h>
#include <FCntl.h>
	
#define   SEEK_SET  0
#define   SEEK_CUR  1
#define   SEEK_END  2
#endif


#ifdef UNIX
#include <stdio.h>
#include <strings.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/file.h>
#include <ctype.h>
#endif




	
/*
#define   SEEK_SET  0
#define   SEEK_CUR  1
#define   SEEK_END  2
*/	




#ifdef UNIX
	static int pmode = 0666;

#define O_BINARY 0
#ifndef NeXT
#define SEEK_SET L_SET
#define SEEK_CUR L_INCR
#define SEEK_END L_XTND
#endif

#endif



#define sign_bit        (-1 ^ 0x7fff)

#define sign_extend(a)  ( ((a) & sign_bit) ? ( (a) | sign_bit) : (a))


/**
char *PathNameFromWD (short , char *);
**/
