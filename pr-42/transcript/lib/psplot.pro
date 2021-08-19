% Start of psplot.pro -- prolog for plot(5) translator
% Copyright (c) 1984,1985,1987 Adobe Systems Incorporated. All Rights Reserved. 
%              RESTRICTED RIGHTS LEGEND
% Use, duplication or disclosure by the Government is subject to
% restrictions as set forth in subdivision (b)(3)(ii) of the Rights in
% Technical Data and Computer Software Clause at 252.227-7013.
% Name of Contractor: Adobe Systems Incorporated
% Address:  1870 Embarcadero Road
%	  Palo Alto, California  94303
% RCS: $Header: psplot.pro,v 2.2 87/11/17 16:41:06 byron Rel $
save 50 dict begin /psplot exch def
/StartPSPlot
   {newpath 0 0 moveto 0 setlinewidth 0 setgray 1 setlinecap
    /imtx matrix currentmatrix def /dmtx matrix defaultmatrix def
    /fnt /Courier findfont def /smtx matrix def fnt 8 scalefont setfont}def
/solid {{}0}def
/dotted	{[2 nail 10 nail ] 0}def
/longdashed {[10 nail] 0}def
/shortdashed {[6 nail] 0}def
/dotdashed {[2 nail 6 nail 10 nail 6 nail] 0}def
/disconnected {{}0}def
/min {2 copy lt{pop}{exch pop}ifelse}def
/max {2 copy lt{exch pop}{pop}ifelse}def
/len {dup mul exch dup mul add sqrt}def
/nail {0 imtx dtransform len 0 idtransform len}def

/m {newpath moveto}def
/n {lineto currentpoint stroke moveto}def
/p {newpath moveto gsave 1 setlinecap solid setdash
    dmtx setmatrix .4 nail setlinewidth
    .05 0 idtransform rlineto stroke grestore}def
/l {moveto lineto currentpoint stroke moveto}def
/t {smtx currentmatrix pop imtx setmatrix show smtx setmatrix}def
/a {gsave newpath /y2 exch def /x2 exch def 
    /y1 exch def /x1 exch def /yc exch def /xc exch def
    /r x1 xc sub dup mul y1 yc sub dup mul add sqrt
       x2 xc sub dup mul y2 yc sub dup mul add sqrt add 2 div def
    /ang1 y1 yc sub x1 xc sub atan def
    /ang2 y2 yc sub x2 xc sub atan def
    xc yc r ang1 ang2 arc stroke grestore}def
/c {gsave newpath 0 360 arc stroke grestore}def
/e {gsave showpage grestore newpath 0 0 moveto}def
/f {load exec setdash}def
/s {/ury exch def /urx exch def /lly exch def /llx exch def
    imtx setmatrix newpath clippath pathbbox newpath
    /dury exch def /durx exch def /dlly exch def /dllx exch def
    /md durx dllx sub dury dlly sub min def
    /Mu urx llx sub ury lly sub max def
    dllx dlly translate md Mu div dup scale llx neg lly neg translate}def
/EndPSPlot {clear psplot end restore}def
