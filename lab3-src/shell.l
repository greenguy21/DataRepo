
/*
 *
 * CS-252 Fall 2013
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <string.h>
#include <string>
#include "y.tab.h"

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
    unput(c);
}

%}

%%

"exit"	{
		return EXIT;
	}

\n 	{
		return NEWLINE;
	}

[ \t] 	{
		/* Discard spaces and tabs */
     	}	

">" 	{
		return GREAT;
	}

">>"	{
		return GREATGREAT;
	}

">&"	{
		return GREATAMP;
	}

">>&"	{
		return GREATGREATAMP;
	}	

"<"	{
		return LESS;
	}

"|"	{
		return PIPE;
	}

"&"	{
		return AMP;
	}

\"[^\n\"]*\" {
		char str[strlen(yytext) - 2];
		memcpy(str, &yytext[1], strlen(yytext) - 2);
		yylval.string_val = strdup(str);
		return WORD;
	}
([^ <>|\t\n]\\[ \\&<>|\t\n])+ {
		yylval.string_val = strdup(yytext);
		return WORD;	
	}

[^ \t\n<>&|][^ \t\n<>&|]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}
%%
