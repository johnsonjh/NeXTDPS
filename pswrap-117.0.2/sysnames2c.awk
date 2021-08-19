/ systemnamemap$/ {
	printf("\twellKnownPSNames = CreatePSWDict(%d);\n\n",$1);
}

/[ \t]*[0-9].* definesystemname/ {
	firstc = substr($2,1,1)
	if (firstc == "/") {
	    opname = substr($2,2)	# take off /
	} else if (firstc == "(") {
	    opname = substr($2,2,length($2) - 2)
	} else {  exit 2; }
	opnum = $1
	printf("\tPSWDictEnter(wellKnownPSNames,\"%s\",\t%d);\n",opname,opnum);
}

