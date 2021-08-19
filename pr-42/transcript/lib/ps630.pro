% Start of ps630.pro -- prolog for Diablo 630 translator
% Copyright (c) 1985,1987 Adobe Systems Incorporated. All Rights Reserved. 
% GOVERNMENT END USERS: See Notice file in TranScript library directory
% -- probably /usr/lib/ps/Notice
% RCS: $Header: ps630.pro,v 2.2 87/11/17 16:40:15 byron Rel $
/maxtabs 159 def
/htabs maxtabs 1 add array def
/JA 514 array def
/MRESET	{/MR 1020 def /ML 18 def}def% paperright paperleft
/MRS {currentpoint pop add /MR exch def}def
/MLS {/ML currentpoint pop def}def
/NM 1 def
/BM 2 def
/SM 4 def
/UM 8 def
/SBM 16 def
/SPM 32 def
/CR{VU ML exch moveto}def
/LF{VU currentpoint pop exch moveto}def
/M/moveto load def
/R {0 rmoveto} def
/PGI{.6 setlinewidth}def
/PG{currentpoint pop showpage PGI}def
/HU{.6 mul}def
/VU{1.5 mul}def
/SP{gsave exec grestore 1 0 rmoveto exec -1 0 rmoveto}def
/UP{currentpoint /y exch def /x exch def
    grestore gsave currentpoint 3 sub newpath moveto x y 3
    sub lineto stroke grestore x  y moveto} def
/SETM{dup NM and 0 ne {NFT setfont}if
      BM and 0 ne {BFT setfont}if
     }def
/GETW{/W 0 def /NC 0 def 0 2 SARR length 1 sub dup
      /nelts exch def 
  	 {dup SARR exch get SETM 1 add SARR exch get
  	  dup length NC add /NC exch def incr 0 ne 
	     {dup length incr mul W add /W exch def}if
  	  stringwidth pop W add /W exch def}for
      W}def
/JUMS{dup SARR exch get dup /mode exch def SETM
       mode SBM and 0 ne{0 PSVMI2 neg rmoveto}if
       mode SPM and 0 ne{0 PSVMI2 rmoveto}if
       mode UM and 0 ne dup{gsave}if exch}def
/JU{/incr exch def /nspaces exch def
    GETW MR currentpoint pop sub exch sub /bs exch def
    ( ) stringwidth pop incr add .5 mul dup nspaces mul bs abs gt
      {pop /incsp bs nspaces div def}
      {bs 0 lt {neg} if
       /incsp exch def bs nspaces incsp mul sub NC 1 sub dup 0 eq{pop 1}if div
       dup abs 4.2 le
         {incr add /incr exch def} {pop /incsp 0 def} ifelse
      }ifelse
    0 2 nelts 
       {JUMS 1 add incsp 0 32 incr 0 6 -1 roll SARR exch get mode SM and 0 ne
	{{awidthshow} 7 copy SP}{awidthshow}ifelse{UP} if} for
   }def
/S{/incr 0 def CTEST mode SM and 0 ne{{show} 2 copy SP}{show}ifelse{UP}if}def
/GSH {gsave show grestore}def
/CTEST{/str exch def dup /mode exch def
       UM and 0 ne dup {gsave} if mode SETM str}def
/AS{/incr exch def CTEST
 incr exch 0 exch mode SM and 0 ne{{ashow} 4 copy SP}{ashow}ifelse{UP}if}def
/CTABALL{htabs /tabs exch def 0 1 maxtabs{tabs exch 999999 put}for}def
/STAB{currentpoint pop htabs
      /tabs exch def round /tabloc exch def tabs dup maxtabs get 999999 eq
      {tabloc TF dup dup tabs exch get tabloc ne
      {1 add maxtabs exch 1 neg exch{tabs exch dup tabs exch 1 sub get put}for
       tabloc put}{pop pop pop}ifelse}{pop}ifelse}def
/CTAB{currentpoint pop htabs
      /tabs exch def round dup/tabloc exch def TF dup tabs exch get tabloc eq
      {1 maxtabs 1 sub{tabs exch 2 copy 1 add get put}for
       tabs maxtabs 999999 put}{pop}ifelse}def
/DOTAB{currentpoint exch true htabs 
       /tabs exch def /sr exch def 1 add TF dup maxtabs le
       {tabs exch get dup 999999 ne
	{sr{exch moveto}{3 -1 roll sub neg moveto}ifelse}{CL}ifelse
       }{CL}ifelse}def
/CL{pop pop sr not{pop}if}def
/TF{/val exch def maxtabs 1 add 0 1 maxtabs
    {dup tabs exch get val ge {exch pop exit}{pop}ifelse}for}def
/AC{/incr exch def GETW neg MR add ML add 2 div currentpoint exch pop moveto
    0 2 nelts{JUMS  1 add  SARR exch get incr exch 0 exch mode SM and 0 ne
     {{ashow}4 copy SP}{ashow}ifelse{UP}if}for}def
/SJA{counttomark JA exch 0 exch getinterval astore exch pop /SARR exch def}def
/RMV{/incr exch def dup dup stringwidth pop PSHMI sub
 exch length 1 sub incr mul add neg 0 rmoveto gsave}def
/RMVBK{grestore PSHMI incr add neg 0 rmoveto}def

