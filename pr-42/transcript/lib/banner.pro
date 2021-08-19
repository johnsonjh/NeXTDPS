%!
% Start of banner.pro -- prolog for printer banner maker
% Copyright (c) 1985,1987 Adobe Systems Incorporated. All Rights Reserved. 
% GOVERNMENT END USERS: See Notice file in TranScript library directory
% -- probably /usr/lib/ps/Notice
% 4.2bsd banner page prolog (uses lpd/"of" short banner string)
% host:user  Job: file  Date: Day Mth dd hh:mm:ss yyyy
% RCSID: $Header: banner.bsd,v 2.2 87/11/17 16:39:27 byron Rel $
/Banner{14 dict begin dup statusdict exch /jobname exch put
 save /sv exch def /jobstr exch def /PN exch def
 jobstr(Job: )search not{0()}if /hu exch def pop
 /s1 exch def s1(Date: )search
   {/jb exch def pop/da exch def}{/jb exch def/da()def}ifelse
 statusdict /printername known
 {/pn 31 string statusdict/printername get exec def}{/pn(PostScript)def}ifelse
 /w 670 def/y{72 w moveto show}def/z{y/w w 30 sub def}def
 /l{gsave .5 setgray 12 setlinewidth newpath moveto
  468 0 rlineto stroke grestore}def
 72 720 l /Helvetica-Bold findfont 14 scalefont setfont
 hu z jb z/Helvetica findfont 14 scalefont setfont da z PN y( / )show pn show 
 72 540 l /Courier findfont 10 scalefont setfont 72 500 moveto
 4{gsave PN show( )show jobstr show grestore 0 -25 rmoveto}repeat
 showpage sv end restore}def
