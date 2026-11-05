%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
%}

%option noyywrap yylineno

DIGIT   [0-9]
LETTER  [a-zA-Z_]

%%

"int"       { return KW_INT; }
"if"        { return KW_IF; }
"else"      { return KW_ELSE; }
"while"     { return KW_WHILE; }
"for"       { return KW_FOR; }
"return"    { return KW_RETURN; }

{DIGIT}+                     { yylval.num = atoi(yytext); return NUM; }
{LETTER}({LETTER}|{DIGIT})*  { yylval.str = strdup(yytext); return ID; }

"=="  { return EQ; }
"!="  { return NE; }
"<="  { return LE; }
">="  { return GE; }

[-+*/<>=;(){},]  { return yytext[0]; }

"//".*        { }
[ \t\r\n]+    { }

.   { fprintf(stderr, "Рядок %d: невідомий символ '%s'\n", yylineno, yytext); }

%%
