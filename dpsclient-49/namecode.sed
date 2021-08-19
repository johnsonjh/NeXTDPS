#n
#
# This sed script converts an system name list from Adobe to a
# format understood by the makeNameTable program.  The conversion is:
#
#	FROM:	<number> /<name> definesystemname
#	  or:	<number> (<name>) cvn definesystemname
#	TO:	<name> <number> -1
#
# The first line of this file prevents unmatched input being copied
# to stdout by default.
#
# The first group of commands handles almost all the lines of the file.
# The second group exists for the "[" and "]" operators, which are
# defined as strings converted to names instead of plain literal names.
#

/[0-9]* *\/[^ ]* *definesystemname/ {
s;\([0-9]*\) */\([^ ]*\) *definesystemname.*;\2 \1 -1;
p
}

/[0-9]* *([^)]*) *cvn *definesystemname/ {
s;\([0-9]*\) *(\([^)]*\)) *cvn *definesystemname.*;\2 \1 -1;
p
}

