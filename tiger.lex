%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"
#include "errormsg.h"

int charPos=1;
int stringPos=0;
char str[4096];
int commentLayers=0;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

void error(char *buf)
{
  EM_error(EM_tokPos, buf);
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it
	return NULL;
}

%}
  /* You can add lex definitions here. */

%Start STR COMMENT STRIGN
%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>"." {adjust(); return DOT;}
<INITIAL>"," {adjust(); return COMMA;}
<INITIAL>":" {adjust(); return COLON;}
<INITIAL>";" {adjust(); return SEMICOLON;}
<INITIAL>"(" {adjust(); return LPAREN;}
<INITIAL>")" {adjust(); return RPAREN;}
<INITIAL>"{" {adjust(); return LBRACE;}
<INITIAL>"}" {adjust(); return RBRACE;}
<INITIAL>"[" {adjust(); return LBRACK;}
<INITIAL>"]" {adjust(); return RBRACK;}
<INITIAL>"*" {adjust(); return TIMES;}
<INITIAL>"/" {adjust(); return DIVIDE;}
<INITIAL>"+" {adjust(); return PLUS;}
<INITIAL>"-" {adjust(); return MINUS;}
<INITIAL>" " {adjust();}
<INITIAL>"&" {adjust(); return AND;}
<INITIAL>"|" {adjust(); return OR;}
<INITIAL>"=" {adjust(); return EQ;}
<INITIAL>"<" {adjust(); return LT;}
<INITIAL>">" {adjust(); return GT;}
<INITIAL>">=" {adjust(); return GE;}
<INITIAL>"<=" {adjust(); return LE;}
<INITIAL>"<>" {adjust(); return NEQ;}
<INITIAL>":=" {adjust(); return ASSIGN;}
<INITIAL>"\n" {adjust(); EM_newline(); continue;}
<INITIAL>"\t" {adjust();}
<INITIAL>\" {stringPos=charPos; adjust(); BEGIN STR;}
<INITIAL>"/*" {adjust(); commentLayers++; BEGIN COMMENT;}
<INITIAL>function {adjust(); return FUNCTION;}
<INITIAL>var {adjust(); return VAR;}
<INITIAL>type {adjust(); return TYPE;}
<INITIAL>while {adjust(); return WHILE;}
<INITIAL>for {adjust(); return FOR;}
<INITIAL>to {adjust(); return TO;}
<INITIAL>do {adjust(); return DO;}
<INITIAL>end {adjust(); return END;}
<INITIAL>if {adjust(); return IF;}
<INITIAL>then {adjust(); return THEN;}
<INITIAL>else {adjust(); return ELSE;}
<INITIAL>in {adjust(); return IN;}
<INITIAL>let {adjust(); return LET;}
<INITIAL>nil {adjust(); return NIL;}
<INITIAL>array {adjust(); return ARRAY;}
<INITIAL>of {adjust(); return OF;}

<INITIAL>[a-zA-Z](([a-zA-Z0-9]|_)*) {adjust(); yylval.sval=String(yytext); return ID;}
<INITIAL>[0-9]+ {adjust(); yylval.ival=atoi(yytext); return INT;}
<INITIAL>. {adjust(); error("illegal character");}

<STR>\" {adjust(); BEGIN INITIAL; EM_tokPos = stringPos; yylval.sval=String(str); memset(str, 0, 4096); return STRING;}
<STR>"\\n" {adjust(); strcat(str, "\n");}
<STR>\\[0-9][0-9][0-9] {adjust(); int i = atoi(++yytext); char a[2]; a[0] = i; strcat(str, a);}
<STR>"\\t" {adjust(); strcat(str, "\t");}
<STR>\\\" {adjust(); strcat(str, "\"");}
<STR>\\\\ {adjust(); strcat(str, "\\");}
<STR>\\\^. {adjust(); int i = *(yytext+2); i = i - 64; char a[2]; a[0] = i; a[1] = 0; strcat(str, a);}
<STR>\\ {adjust(); BEGIN STRIGN;}
<STR>. {adjust(); strcat(str, yytext);}
<STR>EOF {adjust(); error("string not complete");}

<STRIGN>\\ {adjust(); BEGIN STR;}
<STRIGN>"\n" {adjust(); EM_newline(); continue;}
<STRIGN>. {adjust();}
<STRIGN>EOF {adjust(); error("string not complete");}

<COMMENT>"*/" {adjust(); commentLayers--; if(commentLayers == 0) BEGIN INITIAL;}
<COMMENT>"/*" {adjust(); commentLayers++;}
<COMMENT>"\n" {adjust(); EM_newline(); continue;}
<COMMENT>. {adjust();}
<COMMENT>EOF {adjust(); error("comment not complete");}
