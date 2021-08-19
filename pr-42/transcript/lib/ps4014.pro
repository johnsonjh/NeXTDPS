% Start of ps4014.pro -- prolog for Tektronics 4014 translator
% Copyright (c) 1985,1987 Adobe Systems Incorporated. All Rights Reserved. 
% GOVERNMENT END USERS: See Notice file in TranScript library directory
% -- probably /usr/lib/ps/Notice
% RCS: $Header: ps4014.pro,v 2.2 87/11/17 16:40:09 byron Rel $
save /ps4014sav exch def
/inch{72 mul}def
/xHome 0 def
/yHome 3071 def
/xLeftMarg 0 def
/m /moveto load def
/l /lineto load def
/rl /rlineto load def
/r /rmoveto load def
/s /stroke load def
/ml {/sychar exch def AryOfDxSpace sychar get neg 0 rmoveto} def
/mr {/sychar exch def AryOfDxSpace sychar get 0 rmoveto} def
/mu {/sychar exch def 0 AryOfDySpace sychar get rmoveto} def
/md {/sychar exch def 0 AryOfDySpace sychar get neg rmoveto} def
/cr1 {/sychar exch def 
      xLeftMarg currentpoint exch pop AryOfDySpace sychar get sub moveto} def
/cr2 {xLeftMarg currentpoint exch pop moveto} def
/sh /show load def
% scale the coordinate space 
% input: size of image area (x, y) in inches
/ScaleCoords {72 4096 dxWidInch div div 72 3120 dyHtInch div div scale} def
/AryOfDxSpace [56 51 34 31] def
/AryOfDySpace [90 81 53 48] def
/AryOfDyBaseLine [ 0 0 0 0 ] def % filled in at init time
% input: character style index
/SetCharStyle
    {/sychar exch def typeface findfont
    AryOfDySpace sychar get scalefont setfont
    } def
/AryOfDashSolid [] def
/AryOfDashDotted [12 24] def
/AryOfDashDotDash [12 24 96 24] def
/AryOfDashShortDash [24 24] def
/AryOfDashLongDash [144 24] def
/AryOfAryOfDash 
  [AryOfDashSolid AryOfDashDotted AryOfDashDotDash 
   AryOfDashShortDash AryOfDashLongDash] def
% input: line style index
/SetLineStyle
    {AryOfAryOfDash exch get dup length setdash } def
/SetAryOfDyBaseLine
    {/mxT matrix def 
    /fontT typeface findfont def
    0 1 AryOfDyBaseLine length 1 sub
        {/sycharT exch def
        % transform dyBaseline into user space at current font size
        AryOfDyBaseLine sycharT 
            0 fontT /FontBBox get 1 get		% stack = (0 dyBaseline)
                AryOfDySpace sycharT get dup mxT scale 
                fontT /FontMatrix get mxT concatmatrix
            dtransform 
        exch pop put
        } for
    } def
/typeface /Courier def		% Should be fixed pitch
SetAryOfDyBaseLine
% end of 4014 prologue
