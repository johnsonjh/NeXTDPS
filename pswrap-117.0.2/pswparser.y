/*
  pswparser.y

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Andy Shore: Tue Apr 19 21:09:27 1988
Edit History:
Paul Rovner: Tuesday, May 17, 1988 7:52:43 AM
Andrew Shore: Fri Jul  1 10:23:04 1988
Richard Cohn: Mon Sep 26 17:35:15 1988
Bill Bilodeau: Fri Jun 29 17:14:19 1990
End Edit History.

*/

%{

#include "pswpriv.h"
#include "pswsemantics.h"

%}

/* yylval type (from lexer and on stack) */

%union {
    char *object;
    int	intobj;
    Token token;
    Item item;
    Header header;
    int flag;
    Arg arg;
    Subscript subscript;
}


%token <object> DEFINEPS ENDPS STATIC
%token <object> PSCONTEXT
%token <object> BOOLEAN FLOAT DOUBLE UNSIGNED SHORT LONG INT CHAR USEROBJECT
%token <object> NUMSTRING
%token <object> CNAME
%token <intobj> CINTEGER

%token <object> PSNAME PSLITNAME PSREAL PSBOOLEAN PSSTRING PSHEXSTRING
%token <intobj> PSINTEGER
%token <object> PSSUBNAME PSINDEX

%token <object> '(' ')' '|' ';' ',' '*' '[' ']' '{' '}' ':'

%type <token>	Token Tokens Body
%type <item>	Items Item
%type <header>	Header
%type <flag>	Def Type
%type <arg>	InArgs Args ContextArg SubArgs Arg
%type <subscript> Subscript

%start Module

%%

Module:
	/* empty */
	| Module Definition
	;

Definition:
	Header Body ENDPS
		{ FinalizePSWrapDef($1, $2); yyerrok; }
	| error ENDPS
		{ yyerrok; }
	;

Body:
	/* nothing */
		{ $$ = 0; }
	| Tokens
		/* { $$ = $1; }*/
	;

Tokens:
	Token
		{ $$ = AppendPSWToken($1, 0); }
	| Tokens Token
		{ $$ = AppendPSWToken($2, $1); yyerrok; }
	/* | error
		{ $$ = 0; } */
	;

Header:
	Def ')'
		{ $$ = PSWHeader($1, 0, 0); yyerrok; }
	| Def InArgs ')'
		{ $$ = PSWHeader($1, $2, 0); yyerrok; }
	| Def InArgs '|' Args ')'
		{ $$ = PSWHeader($1, $2, $4); yyerrok; }
	| Def '|' Args ')'
		{ $$ = PSWHeader($1, 0, $3); yyerrok; }
	;

Def:
	DEFINEPS CNAME '('
		{ PSWName($2); $$ = 0; yyerrok; } 
	| DEFINEPS STATIC CNAME '('
		{ PSWName($3); $$ = 1; yyerrok; }
	| DEFINEPS error '('
		{ PSWName("error"); $$ = 0; yyerrok; }
	;

Semi:
	/* nothing */
	| ';' { yyerrok; }
	;

InArgs:
	ContextArg Semi
		/* { $$ = $1; } */
	| Args
		/* { $$ = $1; } */
	| ContextArg ';' Args
		{ $$ = ConsPSWArgs($1, $3); }
	;

ContextArg:
	PSCONTEXT CNAME
		{ $$ = PSWArg(T_CONTEXT, PSWItem($2)); }
	;

Args:
	SubArgs Semi
		/* { $$ = $1; }*/
	;

SubArgs:
	Arg
		/* { $$ = $1; }*/
	| SubArgs ';' Arg
		{ yyerrok; $$ = AppendPSWArgs($3, $1); }
	| SubArgs error
	| SubArgs error Arg
		{ yyerrok; $$ = AppendPSWArgs($3, $1); }
	| SubArgs ';' error
	;

Arg: Type Items
		{ $$ = PSWArg($1, $2); yyerrok; }
	;

Items:
	Item
		/* { $$ = $1; } */
	| Items ',' Item
		{ yyerrok; $$ = AppendPSWItems($3, $1); }
	| error { $$ = 0; }
	| Items error
	| Items error Item
		{ yyerrok; $$ = AppendPSWItems($3, $1); }
	| Items ',' error
	;

Item:
	'*' CNAME
		{ $$ = PSWStarItem($2); }
	| CNAME '[' Subscript ']'
		{ $$ = PSWSubscriptItem($1, $3); }
	| CNAME '[' Subscript ']' ':' CNAME
		{ $$ = PSWScaleItem($1, $3, $6, NULL); }
	| CNAME '[' Subscript ']' ':' CINTEGER
		{ $$ = PSWScaleItem($1, $3, NULL, $6); }
	| CNAME
		{ $$ = PSWItem($1); }
	;

Subscript:
	CNAME
		{ $$ = PSWNameSubscript($1); }
	| CINTEGER
		{ $$ = PSWIntegerSubscript($1); }
	;
	
Type:
	BOOLEAN
		{ $$ = T_BOOLEAN; }
	| FLOAT
		{ $$ = T_FLOAT; }
	| DOUBLE
		{ $$ = T_DOUBLE; }
	| CHAR
		{ $$ = T_CHAR; }
	| UNSIGNED CHAR
		{ $$ = T_UCHAR; }
	| INT
		{ $$ = T_INT; }
	| LONG INT
		{ $$ = T_LONGINT; }
	| LONG
		{ $$ = T_LONGINT; }
	| SHORT INT
		{ $$ = T_SHORTINT; }
	| SHORT
		{ $$ = T_SHORTINT; }
	| UNSIGNED
		{ $$ = T_UINT; }
	| UNSIGNED LONG
		{ $$ = T_ULONGINT; }
	| UNSIGNED INT
		{ $$ = T_UINT; }
	| UNSIGNED LONG INT
		{ $$ = T_ULONGINT; }
	| UNSIGNED SHORT
		{ $$ = T_USHORTINT; }
	| UNSIGNED SHORT INT
		{ $$ = T_USHORTINT; }
	| USEROBJECT
		{ $$ = T_USEROBJECT; }
	| NUMSTRING
		{ $$ = T_NUMSTR; }		
	| INT NUMSTRING
		{ $$ = T_NUMSTR; }		
	| FLOAT NUMSTRING
		{ $$ = T_FLOATNUMSTR; }
	| LONG NUMSTRING
		{ $$ = T_LONGNUMSTR; }
	| SHORT NUMSTRING
		{ $$ = T_SHORTNUMSTR; }
	;

Token:
	PSINTEGER
		{ $$ = PSWToken(T_INT, $1); }
	| PSREAL
		{ $$ = PSWToken(T_FLOAT, $1); }
	| PSBOOLEAN
		{ $$ = PSWToken(T_BOOLEAN, $1); }
	| PSSTRING
		{ $$ = PSWToken(T_STRING, $1); }
	| PSHEXSTRING
		{ $$ = PSWToken(T_HEXSTRING, $1); }
	| PSNAME
		{ $$ = PSWToken(T_NAME, $1); }
	| PSLITNAME
		{ $$ = PSWToken(T_LITNAME, $1); }
	| PSSUBNAME PSINDEX
		{ $$ = PSWToken2(T_SUBSCRIPTED, $1, $2); }
	| '[' Body ']'
		{ $$ = PSWToken(T_ARRAY, $2); }
	| '{' Body '}'
		{ $$ = PSWToken(T_PROC, $2); }
	;
