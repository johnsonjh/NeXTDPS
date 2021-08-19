%!PS-Adobe-2.0
%%Title: /usr/lib/NextPrinter/banner.pro -- prolog for printer banner maker
%%DocumentFonts: Helvetica Helvetica-Bold
%%Pages: 1 0
%%EndComments

%
% Copyright (c) 1990 by NeXT, Inc.  All rights reserved
% (Printer) (User) (Job) (Date) Banner --
%
/Banner {
  save
  10 dict begin
    % Save the state of VM
    /SV exch def

    % Save our arguments
    /DATE exch def
    /JOB exch def
    /USER exch def
    /PRNAME exch def

    % Find out the bounding box of the page
    clippath pathbbox
    /URY exch def
    pop pop pop
    
    % Y location for the next line of text
    /Y URY 122 sub def

    % Routine to write a line of text
    /str {
      72 Y moveto
      show
      /Y Y 30 sub def
    } def

    % Routine to draw a bar
    /bar {
      gsave
	.5 setgray
	12 setlinewidth
	newpath
	moveto
	468 0 rlineto
	stroke
      grestore
    } def

    % Draw the top bar
    72 URY 72 sub bar

    % Output the user name and job.
    /Helvetica-Bold findfont 14 scalefont setfont
    USER str
    JOB str

    % Output the data and printer name
    /Helvetica findfont 14 scalefont setfont
    DATE str 
    PRNAME str

    % Draw the lower bar
    72 URY 252 sub bar

    showpage
    SV
  end
  restore
} def

%%EndProlog

