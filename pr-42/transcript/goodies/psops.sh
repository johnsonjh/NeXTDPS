psop "[" "[" <<\mumbleFratz
-- [: mark %start array construction
mumbleFratz
psop "]" "]" <<\mumbleFratz
mark obj0..objn-1 ]: array %end array construction
mumbleFratz
psop "=" "=" <<\mumbleFratz
any =: -- %write text representation of any to standard output file
mumbleFratz
psop "==" "==" <<\mumbleFratz
any ==: -- %write syntactic representation of any to standard output file
mumbleFratz
psop "abs" "abs" <<\mumbleFratz
num1 abs: num2 %absolute value of num1
mumbleFratz
psop "add" "add" <<\mumbleFratz
num1 num2 add: sum %num1 plus num2
mumbleFratz
psop "aload" "aload" <<\mumbleFratz
array aload: a0..anminus1 array %push all elements of array on stack
mumbleFratz
psop "anchorsearch" "anchorsearch" <<\mumbleFratz
string seek anchorsearch: post match true OR string false %determine if seek is initial substring of string
mumbleFratz
psop "and" "and" <<\mumbleFratz
bool1|int1 bool2|int2 and: bool3|int3 %logical | bitwise and
mumbleFratz
psop "arc" "arc" <<\mumbleFratz
x y r ang1 ang2 arc: -- %append counterclockwise arc
mumbleFratz
psop "arcn" "arcn" <<\mumbleFratz
x y r ang1 ang2 arcn: -- %append clockwise arc
mumbleFratz
psop "arcto" "arcto" <<\mumbleFratz
x1 y1 x2 y2 r arcto: xt1 yt1 xt2 yt2 %append tangent arc
mumbleFratz
psop "array" "array" <<\mumbleFratz
int array: array %create array of length int
mumbleFratz
psop "ashow" "ashow" <<\mumbleFratz
ax ay string ashow: -- %add (ax, ay) to width of each char while showing string
mumbleFratz
psop "astore" "astore" <<\mumbleFratz
any0..anynminus1 array astore: array %pop elements from stack into array
mumbleFratz
psop "atan" "atan" <<\mumbleFratz
num den atan: angle %arctangent of num/den in degrees
mumbleFratz
psop "awidthshow" "awidthshow" <<\mumbleFratz
cx cy char ax ay string awidthshow: -- %combined effects of ashow and widthshow
mumbleFratz
psop "banddevice" "banddevice" <<\mumbleFratz
matrix width height proc banddevice: -- %install band buffer device
mumbleFratz
psop "begin" "begin" <<\mumbleFratz
dict begin: -- %push dict on dict stack
mumbleFratz
psop "bind" "bind" <<\mumbleFratz
proc bind: proc %replace operator names in proc by operators
mumbleFratz
psop "bitshift" "bitshift" <<\mumbleFratz
int1 shift bitshift: int2 %bitwise shift of int1 (positive is left)
mumbleFratz
psop "bytesavailable" "bytesavailable" <<\mumbleFratz
file bytesavailable: int %number of bytes available to read
mumbleFratz
psop "cachestatus" "cachestatus" <<\mumbleFratz
-- cachestatus: bsize bmax msize mmax csize cmax blimit %return cache status and parameters
mumbleFratz
psop "ceiling" "ceiling" <<\mumbleFratz
num1 ceiling: num2 %ceiling of num1
mumbleFratz
psop "charpath" "charpath" <<\mumbleFratz
string bool charpath: -- %append character outline to current path
mumbleFratz
psop "clear" "clear" <<\mumbleFratz
|- any1..anyn clear: |- %discard all elements
mumbleFratz
psop "cleartomark" "cleartomark" <<\mumbleFratz
mark obj1..objn cleartomark: -- %discard elements down through mark
mumbleFratz
psop "clip" "clip" <<\mumbleFratz
-- clip: -- %establish new clipping path
mumbleFratz
psop "clippath" "clippath" <<\mumbleFratz
-- clippath: -- %set current path to clipping path
mumbleFratz
psop "closefile" "closefile" <<\mumbleFratz
file closefile: -- %close file
mumbleFratz
psop "closepath" "closepath" <<\mumbleFratz
-- closepath: -- %connect subpath back to its starting point
mumbleFratz
psop "concat" "concat" <<\mumbleFratz
matrix concat: -- %replace CTM by matrix multiply CTM
mumbleFratz
psop "concatmatrix" "concatmatrix" <<\mumbleFratz
matrix1 matrix2 matrix3 concatmatrix: matrix3 %fill matrix3 with matrix1 multiply matrix2
mumbleFratz
psop "stackcopy" "copy" <<\mumbleFratz
any1..anyn n copy: any1..anyn any1..anyn %duplicate top n elements
mumbleFratz
psop "arraycopy" "copy" <<\mumbleFratz
array1 array2 copy: subarray2 %copy elements of array1 to initial subarray of array2
mumbleFratz
psop "dictcopy" "copy" <<\mumbleFratz
dict1 dict2 copy: dict2 %copy contents of dict1 to dict2
mumbleFratz
psop "stringcopy" "copy" <<\mumbleFratz
string1 string2 copy: substring2 %copy elements of string1 to initial substring of string2
mumbleFratz
psop "copypage" "copypage" <<\mumbleFratz
-- copypage: -- %output current page
mumbleFratz
psop "cos" "cos" <<\mumbleFratz
angle cos: real %cosine of angle (degrees)
mumbleFratz
psop "count" "count" <<\mumbleFratz
|- any1..anyn count: |- any1..anyn n %count elements on stack
mumbleFratz
psop "countdictstack" "countdictstack" <<\mumbleFratz
-- countdictstack: int %count elements on dict stack
mumbleFratz
psop "countexecstack" "countexecstack" <<\mumbleFratz
-- countexecstack: int %count elements on exec stack
mumbleFratz
psop "counttomark" "counttomark" <<\mumbleFratz
mark obj1..objn counttomark: mark obj1..objn n %count elements down to mark
mumbleFratz
psop "currentdash" "currentdash" <<\mumbleFratz
-- currentdash: array offset %return current dash pattern
mumbleFratz
psop "currentdict" "currentdict" <<\mumbleFratz
-- currentdict: dict %push current dict on operand stack
mumbleFratz
psop "currentfile" "currentfile" <<\mumbleFratz
-- currentfile: file %return file currently being executed
mumbleFratz
psop "currentflat" "currentflat" <<\mumbleFratz
-- currentflat: num %return current flatness
mumbleFratz
psop "currentfont" "currentfont" <<\mumbleFratz
-- currentfont: font %return current font dictionary
mumbleFratz
psop "currentgray" "currentgray" <<\mumbleFratz
-- currentgray: num %return current gray
mumbleFratz
psop "currenthsbcolor" "currenthsbcolor" <<\mumbleFratz
-- currenthsbcolor: hue sat brt %return current color hue, saturation, brightness
mumbleFratz
psop "currentlinecap" "currentlinecap" <<\mumbleFratz
-- currentlinecap: int %return current line cap
mumbleFratz
psop "currentlinejoin" "currentlinejoin" <<\mumbleFratz
-- currentlinejoin: int %return current line join
mumbleFratz
psop "currentlinewidth" "currentlinewidth" <<\mumbleFratz
-- currentlinewidth: num %return current line width
mumbleFratz
psop "currentmatrix" "currentmatrix" <<\mumbleFratz
matrix currentmatrix: matrix %fill matrix with CTM
mumbleFratz
psop "currentmiterlimit" "currentmiterlimit" <<\mumbleFratz
-- currentmiterlimit: num %return current miter limit
mumbleFratz
psop "currentpoint" "currentpoint" <<\mumbleFratz
-- currentpoint: x y %return current point coordinate
mumbleFratz
psop "currentrgbcolor" "currentrgbcolor" <<\mumbleFratz
-- currentrgbcolor: red green blue %return current color red, green, blue
mumbleFratz
psop "currentscreen" "currentscreen" <<\mumbleFratz
-- currentscreen: freq angle proc %return current halftone screen
mumbleFratz
psop "currenttransfer" "currenttransfer" <<\mumbleFratz
-- currenttransfer: proc %return current transfer function
mumbleFratz
psop "curveto" "curveto" <<\mumbleFratz
x1 y1 x2 y2 x3 y3 curveto: -- %append Bezier cubic section
mumbleFratz
psop "cvi" "cvi" <<\mumbleFratz
num|string cvi: int %convert to integer
mumbleFratz
psop "cvlit" "cvlit" <<\mumbleFratz
any cvlit: any %make object be literal
mumbleFratz
psop "cvn" "cvn" <<\mumbleFratz
string cvn: name %convert to name
mumbleFratz
psop "cvr" "cvr" <<\mumbleFratz
num|string cvr: real %convert to real
mumbleFratz
psop "cvrs" "cvrs" <<\mumbleFratz
num radix string cvrs: substring %convert to string with radix
mumbleFratz
psop "cvs" "cvs" <<\mumbleFratz
any string cvs: substring %convert to string
mumbleFratz
psop "cvx" "cvx" <<\mumbleFratz
any cvx: any %make object be executable
mumbleFratz
psop "def" "def" <<\mumbleFratz
key value def: -- %associate key and value in current dict
mumbleFratz
psop "defaultmatrix" "defaultmatrix" <<\mumbleFratz
matrix defaultmatrix: matrix %fill matrix with device default matrix
mumbleFratz
psop "definefont" "definefont" <<\mumbleFratz
key font definefont: font %register font as a font dictionary
mumbleFratz
psop "dict" "dict" <<\mumbleFratz
int dict: dict %create dictionary with capacity for int elements
mumbleFratz
psop "dictfull" "dictfull" <<\mumbleFratz
 dictfull:  %no more room in dictionary
mumbleFratz
psop "dictstack" "dictstack" <<\mumbleFratz
array dictstack: subarray %copy dict stack into array
mumbleFratz
psop "dictstackoverflow" "dictstackoverflow" <<\mumbleFratz
 dictstackoverflow:  %too many begins
mumbleFratz
psop "dictstackunderflow" "dictstackunderflow" <<\mumbleFratz
 dictstackunderflow:  %too many ends
mumbleFratz
psop "div" "div" <<\mumbleFratz
num1 num2 div: quotient %num1 divided by num2
mumbleFratz
psop "dtransform" "dtransform" <<\mumbleFratz
dx dy dtransform: dx' dy' %transform distance (dx, dy) by CTM
mumbleFratz
psop "dtransform" "dtransform" <<\mumbleFratz
dx dy matrix dtransform: dx' dy' %transform distance (dx, dy) by matrix
mumbleFratz
psop "dup" "dup" <<\mumbleFratz
any dup: any any %duplicate top element
mumbleFratz
psop "echo" "echo" <<\mumbleFratz
bool echo: -- %turn on/off echoing
mumbleFratz
psop "end" "end" <<\mumbleFratz
-- end: -- %pop dict stack
mumbleFratz
psop "eoclip" "eoclip" <<\mumbleFratz
-- eoclip: -- %clip using even-odd inside rule
mumbleFratz
psop "eofill" "eofill" <<\mumbleFratz
-- eofill: -- %fill using even-odd rule
mumbleFratz
psop "eq" "eq" <<\mumbleFratz
any1 any2 eq: bool %test equal
mumbleFratz
psop "erasepage" "erasepage" <<\mumbleFratz
-- erasepage: -- %paint current page white
mumbleFratz
psop "errordict" "errordict" <<\mumbleFratz
-- errordict: dict %push errordict on operand stack
mumbleFratz
psop "exch" "exch" <<\mumbleFratz
any1 any2 exch: any2 any1 %exchange top two elements
mumbleFratz
psop "exec" "exec" <<\mumbleFratz
any exec: -- %execute arbitrary object
mumbleFratz
psop "execstack" "execstack" <<\mumbleFratz
array execstack: subarray %copy exec stack into array
mumbleFratz
psop "execstackoverflow" "execstackoverflow" <<\mumbleFratz
 execstackoverflow:  %exec nesting too deep
mumbleFratz
psop "executeonly" "executeonly" <<\mumbleFratz
array|file|string executeonly: array|file|string %reduce access to execute-only
mumbleFratz
psop "exit" "exit" <<\mumbleFratz
-- exit: -- %exit innermost active loop
mumbleFratz
psop "exp" "exp" <<\mumbleFratz
base exponent exp: real %raise base to exponent power
mumbleFratz
psop "false" "false" <<\mumbleFratz
-- false: false %push boolean value false
mumbleFratz
psop "file" "file" <<\mumbleFratz
string1 string2 file: file %open file identified by string1 with access string2
mumbleFratz
psop "fill" "fill" <<\mumbleFratz
-- fill: -- %fill current path with current color
mumbleFratz
psop "findfont" "findfont" <<\mumbleFratz
key findfont: font %return font dict identified by key
mumbleFratz
psop "flattenpath" "flattenpath" <<\mumbleFratz
-- flattenpath: -- %convert curves to sequences of straight lines
mumbleFratz
psop "floor" "floor" <<\mumbleFratz
num1 floor: num2 %floor of num1
mumbleFratz
psop "flush" "flush" <<\mumbleFratz
-- flush: -- %send buffered data to standard output file
mumbleFratz
psop "flushfile" "flushfile" <<\mumbleFratz
file flushfile: -- %send buffered data or read to EOF
mumbleFratz
psop "FontDirectory" "FontDirectory" <<\mumbleFratz
-- FontDirectory: dict %dictionary of font dictionaries
mumbleFratz
psop "for" "for" <<\mumbleFratz
init incr limit proc for: -- %execute proc with values from init by steps of incr to limit
mumbleFratz
psop "arrayforall" "forall" <<\mumbleFratz
array  proc forall: -- %execute proc for each element of array
mumbleFratz
psop "dictforall" "forall" <<\mumbleFratz
dict  proc forall: -- %execute proc for each element of dict
mumbleFratz
psop "stringforall" "forall" <<\mumbleFratz
string  proc forall: -- %execute proc for each element of string
mumbleFratz
psop "framedevice" "framedevice" <<\mumbleFratz
matrix width height proc framedevice: -- %install frame buffer device
mumbleFratz
psop "ge" "ge" <<\mumbleFratz
num1|str1 num2|str2 ge: bool %test greater or equal
mumbleFratz
psop "arrayget" "get" <<\mumbleFratz
array index get: any %get array element indexed by index
mumbleFratz
psop "dictget" "get" <<\mumbleFratz
dict key get: any %get value associated with key in dict
mumbleFratz
psop "stringget" "get" <<\mumbleFratz
string index get: int %get string element indexed by index
mumbleFratz
psop "arraygetinterval" "getinterval" <<\mumbleFratz
array index count getinterval: subarray %subarray of array starting at index for count elements
mumbleFratz
psop "stringgetinterval" "getinterval" <<\mumbleFratz
string index count getinterval: substring %substring of string starting at index for count elements
mumbleFratz
psop "grestore" "grestore" <<\mumbleFratz
-- grestore: -- %restore graphics state
mumbleFratz
psop "grestoreall" "grestoreall" <<\mumbleFratz
-- grestoreall: -- %restore to bottommost graphics state
mumbleFratz
psop "gsave" "gsave" <<\mumbleFratz
-- gsave: -- %save graphics state
mumbleFratz
psop "gt" "gt" <<\mumbleFratz
num1|str1 num2|str2 gt: bool %test greater than
mumbleFratz
psop "handleerror" "handleerror" <<\mumbleFratz
 handleerror:  %called to report error information
mumbleFratz
psop "identmatrix" "identmatrix" <<\mumbleFratz
matrix identmatrix: matrix %fill matrix with identity transform
mumbleFratz
psop "idiv" "idiv" <<\mumbleFratz
int1 int2 idiv: quotient %integer divide
mumbleFratz
psop "idtransform" "idtransform" <<\mumbleFratz
dx' dy' idtransform: dx dy %inverse transform distance (dx', dy') by CTM
mumbleFratz
psop "idtransform" "idtransform" <<\mumbleFratz
dx' dy' matrix idtransform: dx dy %inverse transform distance (dx', dy') by matrix
mumbleFratz
psop "if" "if" <<\mumbleFratz
bool proc if: -- %execute proc if bool is true
mumbleFratz
psop "ifelse" "ifelse" <<\mumbleFratz
bool proc1 proc2 ifelse: -- %execute proc1 if bool is true, proc2 if bool is false
mumbleFratz
psop "image" "image" <<\mumbleFratz
width height bits/sample matrix proc image: -- %render sampled image onto current page
mumbleFratz
psop "imagemask" "imagemask" <<\mumbleFratz
width height invert matrix proc imagemask: -- %render mask onto current page
mumbleFratz
psop "index" "index" <<\mumbleFratz
anyn..any0 n index: anyn..any0 anyn %duplicate arbitrary element
mumbleFratz
psop "initclip" "initclip" <<\mumbleFratz
-- initclip: -- %set clip path to device default
mumbleFratz
psop "initgraphics" "initgraphics" <<\mumbleFratz
-- initgraphics: -- %reset graphics state parameters
mumbleFratz
psop "initmatrix" "initmatrix" <<\mumbleFratz
-- initmatrix: -- %set CTM to device default
mumbleFratz
psop "interrupt" "interrupt" <<\mumbleFratz
 interrupt:  %external interrupt request (e.g., control-C)
mumbleFratz
psop "invalidaccess" "invalidaccess" <<\mumbleFratz
 invalidaccess:  %attempt to violate access attribute
mumbleFratz
psop "invalidexit" "invalidexit" <<\mumbleFratz
 invalidexit:  %exit not in loop
mumbleFratz
psop "invalidfileaccess" "invalidfileaccess" <<\mumbleFratz
 invalidfileaccess:  %unacceptable access string
mumbleFratz
psop "invalidfont" "invalidfont" <<\mumbleFratz
 invalidfont:  %invalid font name or dict
mumbleFratz
psop "invalidrestore" "invalidrestore" <<\mumbleFratz
 invalidrestore:  %improper restore
mumbleFratz
psop "invertmatrix" "invertmatrix" <<\mumbleFratz
matrix1 matrix2 invertmatrix: matrix2 %fill matrix2 with inverse of matrix1
mumbleFratz
psop "ioerror" "ioerror" <<\mumbleFratz
 ioerror:  %input/output error occurred
mumbleFratz
psop "itransform" "itransform" <<\mumbleFratz
x' y' itransform: x y %inverse transform (x', y') by CTM
mumbleFratz
psop "itransform" "itransform" <<\mumbleFratz
x' y' matrix itransform: x y %inverse transform (x', y') by matrix
mumbleFratz
psop "known" "known" <<\mumbleFratz
dict key known: bool %test whether key is in dict
mumbleFratz
psop "kshow" "kshow" <<\mumbleFratz
proc string kshow: -- %execute proc between characters shown from string
mumbleFratz
psop "le" "le" <<\mumbleFratz
num1|str1 num2|str2 le: bool %test less or equal
mumbleFratz
psop "arraylength" "length" <<\mumbleFratz
array length: int %number of elements in array
mumbleFratz
psop "dictlength" "length" <<\mumbleFratz
dict length: int %number of key-value pairs in dict
mumbleFratz
psop "stringlength" "length" <<\mumbleFratz
string length: int %number of elements in string
mumbleFratz
psop "limitcheck" "limitcheck" <<\mumbleFratz
 limitcheck:  %implementation limit exceeded
mumbleFratz
psop "lineto" "lineto" <<\mumbleFratz
x y lineto: -- %append straight line to (x, y)
mumbleFratz
psop "ln" "ln" <<\mumbleFratz
num ln: real %natural logarithm (base e)
mumbleFratz
psop "load" "load" <<\mumbleFratz
key load: value %search dict stack for key and return associated value
mumbleFratz
psop "log" "log" <<\mumbleFratz
num log: real %logarithm (base 10)
mumbleFratz
psop "loop" "loop" <<\mumbleFratz
proc loop: -- %execute proc an indefinite number of times
mumbleFratz
psop "lt" "lt" <<\mumbleFratz
num1|str1 num2|str2 lt: bool %test less than
mumbleFratz
psop "makefont" "makefont" <<\mumbleFratz
font matrix makefont: font' %transform font by matrix to produce new font'
mumbleFratz
psop "mark" "mark" <<\mumbleFratz
-- mark: mark %push mark on stack
mumbleFratz
psop "matrix" "matrix" <<\mumbleFratz
-- matrix: matrix %create identity matrix
mumbleFratz
psop "maxlength" "maxlength" <<\mumbleFratz
dict maxlength: int %capacity of dict
mumbleFratz
psop "mod" "mod" <<\mumbleFratz
int1 int2 mod: remainder %int1 mod int2
mumbleFratz
psop "moveto" "moveto" <<\mumbleFratz
x y moveto: -- %set current point to (x, y)
mumbleFratz
psop "mul" "mul" <<\mumbleFratz
num1 num2 mul: product %num1 times num2
mumbleFratz
psop "ne" "ne" <<\mumbleFratz
any1 any2 ne: bool %test not equal
mumbleFratz
psop "neg" "neg" <<\mumbleFratz
num1 neg: num2 %negative of num1
mumbleFratz
psop "newpath" "newpath" <<\mumbleFratz
-- newpath: -- %initialize current path to be empty
mumbleFratz
psop "noaccess" "noaccess" <<\mumbleFratz
array|dict|file|string noaccess: array|dict|file|string %disallow any access
mumbleFratz
psop "nocurrentpoint" "nocurrentpoint" <<\mumbleFratz
 nocurrentpoint:  %current point is undefined
mumbleFratz
psop "not" "not" <<\mumbleFratz
bool1|int1 not: bool2|int2 %logical | bitwise not
mumbleFratz
psop "null" "null" <<\mumbleFratz
-- null: null %push null on operand stack
mumbleFratz
psop "nulldevice" "nulldevice" <<\mumbleFratz
-- nulldevice: -- %install no-output device
mumbleFratz
psop "or" "or" <<\mumbleFratz
bool1|int1 bool2|int2 or: bool3|int3 %logical | bitwise inclusive or
mumbleFratz
psop "pathbbox" "pathbbox" <<\mumbleFratz
-- pathbbox: llx lly urx ury %return bounding box of current path
mumbleFratz
psop "pathforall" "pathforall" <<\mumbleFratz
move line curve close pathforall: -- %enumerate current path
mumbleFratz
psop "pop" "pop" <<\mumbleFratz
any pop: -- %discard top element
mumbleFratz
psop "print" "print" <<\mumbleFratz
string print: -- %write characters of string to standard output file
mumbleFratz
psop "prompt" "prompt" <<\mumbleFratz
-- prompt: -- %executed when ready for interactive input
mumbleFratz
psop "pstack" "pstack" <<\mumbleFratz
|- any1 .. anyn pstack: |- any1 .. anyn %print stack nondestructively using ==
mumbleFratz
psop "arrayput" "put" <<\mumbleFratz
array index any put: -- %put any into array at index
mumbleFratz
psop "dictput" "put" <<\mumbleFratz
dict key value put: -- %associate key with value in dict
mumbleFratz
psop "stringput" "put" <<\mumbleFratz
string index int put: -- %put int into string at index
mumbleFratz
psop "arrayputinterval" "putinterval" <<\mumbleFratz
array1 index array2 putinterval: -- %replace subarray of array1 starting at index by array2
mumbleFratz
psop "stringputinterval" "putinterval" <<\mumbleFratz
string1 index string2 putinterval: -- %replace substring of string1 starting at index by string2
mumbleFratz
psop "quit" "quit" <<\mumbleFratz
-- quit: -- %terminate interpreter
mumbleFratz
psop "rand" "rand" <<\mumbleFratz
-- rand: int %generate pseudo-random integer
mumbleFratz
psop "rangecheck" "rangecheck" <<\mumbleFratz
 rangecheck:  %operand out of bounds
mumbleFratz
psop "rcheck" "rcheck" <<\mumbleFratz
array|dict|file|string rcheck: bool %test read access
mumbleFratz
psop "rcurveto" "rcurveto" <<\mumbleFratz
dx1 dy1 dx2 dy2 dx3 dy3 rcurveto: -- %relative curveto
mumbleFratz
psop "read" "read" <<\mumbleFratz
file read: int true OR false %read one character from file
mumbleFratz
psop "readhexstring" "readhexstring" <<\mumbleFratz
file string readhexstring: substring bool %read hex from file into string
mumbleFratz
psop "readline" "readline" <<\mumbleFratz
file string readline: substring bool %read line from file into string
mumbleFratz
psop "readonly" "readonly" <<\mumbleFratz
array|dict|file|string readonly: array|dict|file|string %reduce access to read-only
mumbleFratz
psop "readstring" "readstring" <<\mumbleFratz
file string readstring: substring bool %read string from file
mumbleFratz
psop "renderbands" "renderbands" <<\mumbleFratz
proc renderbands: -- %enumerate bands for output to device
mumbleFratz
psop "repeat" "repeat" <<\mumbleFratz
int proc repeat: -- %execute proc int times
mumbleFratz
psop "resetfile" "resetfile" <<\mumbleFratz
file resetfile: -- %discard buffered characters
mumbleFratz
psop "restore" "restore" <<\mumbleFratz
save restore: -- %restore VM snapshot
mumbleFratz
psop "reversepath" "reversepath" <<\mumbleFratz
-- reversepath: -- %reverse direction of current path
mumbleFratz
psop "rlineto" "rlineto" <<\mumbleFratz
dx dy rlineto: -- %relative lineto
mumbleFratz
psop "rmoveto" "rmoveto" <<\mumbleFratz
dx dy rmoveto: -- %relative moveto
mumbleFratz
psop "roll" "roll" <<\mumbleFratz
anminus1..a0 n j roll: a(jminus1) mod n..a0 anminus1..aj mod n %roll n elements up j times
mumbleFratz
psop "rotate" "rotate" <<\mumbleFratz
angle rotate: -- %rotate user space by angle degrees
mumbleFratz
psop "rotate" "rotate" <<\mumbleFratz
angle matrix rotate: matrix %define rotation by angle degrees
mumbleFratz
psop "round" "round" <<\mumbleFratz
num1 round: num2 %round num1 to nearest integer
mumbleFratz
psop "rrand" "rrand" <<\mumbleFratz
-- rrand: int %return random number seed
mumbleFratz
psop "run" "run" <<\mumbleFratz
string run: -- %execute contents of named file
mumbleFratz
psop "save" "save" <<\mumbleFratz
-- save: save %create VM snapshot
mumbleFratz
psop "scale" "scale" <<\mumbleFratz
sx sy scale: -- %scale user space by sx and sy
mumbleFratz
psop "scale" "scale" <<\mumbleFratz
sx sy matrix scale: matrix %define scaling by sx and sy
mumbleFratz
psop "scalefont" "scalefont" <<\mumbleFratz
font scale scalefont: font' %scale font by scale to produce new font'
mumbleFratz
psop "search" "search" <<\mumbleFratz
string seek search: post match pre true OR string false %search for seek in string
mumbleFratz
psop "setcachedevice" "setcachedevice" <<\mumbleFratz
wx wy llx lly urx ury setcachedevice: -- %declare cached character metrics
mumbleFratz
psop "setcachelimit" "setcachelimit" <<\mumbleFratz
num setcachelimit: -- %set max bytes in cached character
mumbleFratz
psop "setcharwidth" "setcharwidth" <<\mumbleFratz
wx wy setcharwidth: -- %declare uncached character metrics
mumbleFratz
psop "setdash" "setdash" <<\mumbleFratz
array offset setdash: -- %set dash pattern for stroking
mumbleFratz
psop "setflat" "setflat" <<\mumbleFratz
num setflat: -- %set flatness tolerance
mumbleFratz
psop "setfont" "setfont" <<\mumbleFratz
font setfont: -- %set font dictionary
mumbleFratz
psop "setgray" "setgray" <<\mumbleFratz
num setgray: -- %set color to gray value from 0 (black) to 1 (white)
mumbleFratz
psop "sethsbcolor" "sethsbcolor" <<\mumbleFratz
hue sat brt sethsbcolor: -- %set color given hue, saturation, brightness
mumbleFratz
psop "setlinecap" "setlinecap" <<\mumbleFratz
int setlinecap: -- %set shape of line ends for stroke (0=butt, 1=round, 2=square)
mumbleFratz
psop "setlinejoin" "setlinejoin" <<\mumbleFratz
int setlinejoin: -- %set shape of corners for stroke (0=miter, 1=round, 2=bevel)
mumbleFratz
psop "setlinewidth" "setlinewidth" <<\mumbleFratz
num setlinewidth: -- %set line width
mumbleFratz
psop "setmatrix" "setmatrix" <<\mumbleFratz
matrix setmatrix: -- %replace CTM by matrix
mumbleFratz
psop "setmiterlimit" "setmiterlimit" <<\mumbleFratz
num setmiterlimit: -- %set miter length limit
mumbleFratz
psop "setrgbcolor" "setrgbcolor" <<\mumbleFratz
red green blue setrgbcolor: -- %set color given red, green, blue
mumbleFratz
psop "setscreen" "setscreen" <<\mumbleFratz
freq angle proc setscreen: -- %set halftone screen
mumbleFratz
psop "settransfer" "settransfer" <<\mumbleFratz
proc settransfer: -- %set gray transfer function
mumbleFratz
psop "show" "show" <<\mumbleFratz
string show: -- %print characters of string on page
mumbleFratz
psop "showpage" "showpage" <<\mumbleFratz
-- showpage: -- %output and reset current page
mumbleFratz
psop "sin" "sin" <<\mumbleFratz
angle sin: real %sine of angle (degrees)
mumbleFratz
psop "sqrt" "sqrt" <<\mumbleFratz
num sqrt: real %square root of num
mumbleFratz
psop "srand" "srand" <<\mumbleFratz
int srand: -- %set random number seed
mumbleFratz
psop "stack" "stack" <<\mumbleFratz
|- any1 .. anyn stack: |- any1 .. anyn %print stack nondestructively using =
mumbleFratz
psop "stackoverflow" "stackoverflow" <<\mumbleFratz
 stackoverflow:  %operand stack overflow
mumbleFratz
psop "stackunderflow" "stackunderflow" <<\mumbleFratz
 stackunderflow:  %operand stack underflow
mumbleFratz
psop "StandardEncoding" "StandardEncoding" <<\mumbleFratz
-- StandardEncoding: array %standard font encoding vector
mumbleFratz
psop "start" "start" <<\mumbleFratz
-- start: -- %executed at interpreter startup
mumbleFratz
psop "status" "status" <<\mumbleFratz
file status: bool %return status of file
mumbleFratz
psop "stop" "stop" <<\mumbleFratz
-- stop: -- %terminate stopped context
mumbleFratz
psop "stopped" "stopped" <<\mumbleFratz
any stopped: bool %establish context for catching stop
mumbleFratz
psop "store" "store" <<\mumbleFratz
key value store: -- %replace topmost definition of key
mumbleFratz
psop "string" "string" <<\mumbleFratz
int string: string %create string of length int
mumbleFratz
psop "stringwidth" "stringwidth" <<\mumbleFratz
string stringwidth: wx wy %width of string in current font
mumbleFratz
psop "stroke" "stroke" <<\mumbleFratz
-- stroke: -- %draw line along current path
mumbleFratz
psop "strokepath" "strokepath" <<\mumbleFratz
-- strokepath: -- %compute outline of stroked path
mumbleFratz
psop "sub" "sub" <<\mumbleFratz
num1 num2 sub: difference %num1 minus num2
mumbleFratz
psop "syntaxerror" "syntaxerror" <<\mumbleFratz
 syntaxerror:  %syntax error in PS program text
mumbleFratz
psop "systemdict" "systemdict" <<\mumbleFratz
-- systemdict: dict %push systemdict on operand stack
mumbleFratz
psop "timeout" "timeout" <<\mumbleFratz
 timeout:  %time limit exceeded
mumbleFratz
psop "filetoken" "token" <<\mumbleFratz
file token: token true OR false %read token from file
mumbleFratz
psop "stringtoken" "token" <<\mumbleFratz
string token: post token true OR false %read token from start of string
mumbleFratz
psop "transform" "transform" <<\mumbleFratz
x y transform: x' y' %transform (x, y) by CTM
mumbleFratz
psop "transform" "transform" <<\mumbleFratz
x y matrix transform: x' y' %transform (x, y) by matrix
mumbleFratz
psop "translate" "translate" <<\mumbleFratz
tx ty translate: -- %translate user space by (tx, ty)
mumbleFratz
psop "translate" "translate" <<\mumbleFratz
tx ty matrix translate: matrix %define translation by (tx, ty)
mumbleFratz
psop "true" "true" <<\mumbleFratz
-- true: true %push boolean value true
mumbleFratz
psop "truncate" "truncate" <<\mumbleFratz
num1 truncate: num2 %remove fractional part of num1
mumbleFratz
psop "type" "type" <<\mumbleFratz
any type: name %return name identifying any's type
mumbleFratz
psop "typecheck" "typecheck" <<\mumbleFratz
 typecheck:  %operand of wrong type
mumbleFratz
psop "undefined" "undefined" <<\mumbleFratz
 undefined:  %name not known
mumbleFratz
psop "undefinedfilename" "undefinedfilename" <<\mumbleFratz
 undefinedfilename:  %file not found
mumbleFratz
psop "undefinedresult" "undefinedresult" <<\mumbleFratz
 undefinedresult:  %over/underflow or meaningless result
mumbleFratz
psop "unmatchedmark" "unmatchedmark" <<\mumbleFratz
 unmatchedmark:  %expected mark not on stack
mumbleFratz
psop "unregistered" "unregistered" <<\mumbleFratz
 unregistered:  %internal error
mumbleFratz
psop "userdict" "userdict" <<\mumbleFratz
-- userdict: dict %push userdict on operand stack
mumbleFratz
psop "usertime" "usertime" <<\mumbleFratz
-- usertime: int %return time in milliseconds
mumbleFratz
psop "version" "version" <<\mumbleFratz
-- version: string %interpreter version
mumbleFratz
psop "VMerror" "VMerror" <<\mumbleFratz
 VMerror:  %VM exhausted
mumbleFratz
psop "vmstatus" "vmstatus" <<\mumbleFratz
-- vmstatus: level used maximum %report VM status
mumbleFratz
psop "wcheck" "wcheck" <<\mumbleFratz
array|dict|file|string wcheck: bool %test write access
mumbleFratz
psop "where" "where" <<\mumbleFratz
key where: dict true OR false %find dict in which key is defined
mumbleFratz
psop "widthshow" "widthshow" <<\mumbleFratz
cx cy char string widthshow: -- %add (cx, cy) to width of char while showing string
mumbleFratz
psop "write" "write" <<\mumbleFratz
file int write: -- %write one character to file
mumbleFratz
psop "writehexstring" "writehexstring" <<\mumbleFratz
file string writehexstring: -- %write string to file as hex
mumbleFratz
psop "writestring" "writestring" <<\mumbleFratz
file string writestring: -- %write characters of string to file
mumbleFratz
psop "xcheck" "xcheck" <<\mumbleFratz
any xcheck: bool %test executable attribute
mumbleFratz
psop "xor" "xor" <<\mumbleFratz
bool1|int1 bool2|int2 xor: bool3|int3 %logical | bitwise exclusive or
mumbleFratz
