%{
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"

int charPos=1;

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

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/
int comment_count = 0;

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	char *buf;
  int i, j;
  int len = strlen(str);

  if (len == 2) {
    buf = "";
    return buf;
  }

  buf = malloc(1024);
  i = 1;
  j = 0;
  while (i < len-1) {
    if (i < len - 2 && str[i] == '\\') {
      if (str[i+1] == 'n') {
        buf[j] = '\n';
        i += 2;
      } else if (str[i+1] == 't') {
        buf[j] = '\t';
        i += 2;
      } else if (str[i+1] >= '0' && str[i+1] <= '9') {
        char c = (str[i+1]-'0')*10*10+(str[i+2]-'0')*10+str[i+3]-'0';
        buf[j] = c;
        i += 4;
      } else if (str[i+1] == '\\') {
        buf[j] = '\\';
        i += 2;
      } else if (str[i+1] == '\"') {
        buf[j] = '\"';
        i += 2;
      } else if (str[i+1] == '^') {
        buf[j] = str[i+2] - 'A' + 1;
        i += 3;
      } else {
        i++;
        while (i < len && str[i] != '\\') i++;
        i++;
        j--;
      }
    } else {
      buf[j] = str[i];
      i++;
    }
    j++;
  }
  buf[j] = '\0';

	return buf;
}

%}
  /* You can add lex definitions here. */

%Start COMMENT

%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

[ \t]+ {adjust();}
"\n" {adjust();EM_newline();}
<INITIAL>"," {adjust(); return COMMA;}
<INITIAL>":" {adjust(); return COLON;}
<INITIAL>";" {adjust(); return SEMICOLON;}
<INITIAL>"(" {adjust(); return LPAREN;}
<INITIAL>")" {adjust(); return RPAREN;}
<INITIAL>"[" {adjust(); return LBRACK;}
<INITIAL>"]" {adjust(); return RBRACK;}
<INITIAL>"{" {adjust(); return LBRACE;}
<INITIAL>"}" {adjust(); return RBRACE;}
<INITIAL>"." {adjust(); return DOT;}
<INITIAL>"+" {adjust(); return PLUS;}
<INITIAL>"-" {adjust(); return MINUS;}
<INITIAL>"*" {adjust(); return TIMES;}
<INITIAL>"/" {adjust(); return DIVIDE;}
<INITIAL>"=" {adjust(); return EQ;}
<INITIAL>"<>" {adjust(); return NEQ;}
<INITIAL>"<" {adjust(); return LT;}
<INITIAL>"<=" {adjust(); return LE;}
<INITIAL>">" {adjust(); return GT;}
<INITIAL>">=" {adjust(); return GE;}
<INITIAL>"&" {adjust(); return AND;}
<INITIAL>"|" {adjust(); return OR;}
<INITIAL>":=" {adjust(); return ASSIGN;}
<INITIAL>"array" {adjust(); return ARRAY;}
<INITIAL>"if" {adjust(); return IF;}
<INITIAL>"then" {adjust(); return THEN;}
<INITIAL>"else" {adjust(); return ELSE;}
<INITIAL>"while" {adjust(); return WHILE;}
<INITIAL>"for" {adjust(); return FOR;}
<INITIAL>"to" {adjust(); return TO;}
<INITIAL>"do" {adjust(); return DO;}
<INITIAL>"let" {adjust(); return LET;}
<INITIAL>"in" {adjust(); return IN;}
<INITIAL>"end" {adjust(); return END;}
<INITIAL>"of" {adjust(); return OF;}
<INITIAL>"break" {adjust(); return BREAK;}
<INITIAL>"nil" {adjust(); return NIL;}
<INITIAL>"function" {adjust(); return FUNCTION;}
<INITIAL>"var" {adjust(); return VAR;}
<INITIAL>"type" {adjust(); return TYPE;}

<INITIAL>\"(\\\"|[^"])*\" {
  adjust();
  yylval.sval = getstr(yytext);
  return STRING;
}
<INITIAL>[A-Za-z][A-Za-z0-9_]* {adjust(); yylval.sval = strdup(yytext); return ID;}
<INITIAL>[0-9]+ {adjust(); yylval.ival = atoi(yytext); return INT;}

<INITIAL>"/*" {adjust(); comment_count++; BEGIN COMMENT;}
<COMMENT>"/*" {adjust(); comment_count++;}
<COMMENT>"*/" {
  adjust(); 
  comment_count--;
  if (comment_count == 0) {
    BEGIN INITIAL;
  }
}
<COMMENT>. {adjust();}
