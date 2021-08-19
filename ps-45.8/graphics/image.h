/*
  image.h

Copyright (c) 1986, 1987 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: 
Edit History:
Ivor Durham: Tue Dec  2 15:21:09 1986
Ed Taft: Sat Jul 11 13:07:20 1987
Linda Gass: Mon Nov 30 15:22:02 1987
Joe Pasqua: Thu Jan 12 12:47:35 1989
End Edit History.
*/

#ifndef	IMAGE_H
#define	IMAGE_H

#include BASICTYPES
#include ENVIRONMENT

#define _image extern

/* Exported Procedures */

_image procedure DoImageMark();

/* Exported Data */

_image integer imageNum;

#endif	IMAGE_H
