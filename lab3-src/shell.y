
/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	NOTOKEN EXIT GREAT GREATGREAT GREATAMP GREATGREATAMP NEWLINE LESS PIPE AMP

%union	{
		char   *string_val;
	}

%{
//#define yylex yylex
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <regex.h>
#include <dirent.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include "command.h"

#define MAXFILENAME 1024

void yyerror(const char * s);
int yylex();
bool comp (char* i,char* j);
void expandWildcards(char* prefix, char* arg){
	//check if suffix is empty
	if (arg[0]== 0) {
		Command::_currentSimpleCommand->insertArgument(strdup(prefix));
		return;
	}
	
	//obtain the next suffix segment
	char * s = strchr(arg, '/');
	char component[MAXFILENAME];
	if (s!=NULL){ 
		strncpy(component,arg, s-arg);
		arg = s + 1;
	}
	else {
		strcpy(component, arg);
		arg = arg + strlen(arg);
	}
	//printf("%s\n", arg);
	char newPrefix[MAXFILENAME];
	//Return if no '*' or '?'
	if (strchr(component, '*') == NULL && strchr(component, '?') == NULL){
		if(prefix == NULL || prefix == '\0') prefix = (char*)"";
		if (prefix == NULL){
			if (arg[0] == 0){
				sprintf(newPrefix,"%s", component); 
			} else {
				sprintf(newPrefix,"%s/", component); 
			}
			//sprintf(newPrefix,"%s", component); 
		} else {
			if (arg[0] == 0){
				sprintf(newPrefix,"%s%s", prefix, component); 
			} else {
				sprintf(newPrefix,"%s%s/", prefix, component); 
			}
		}

		//sprintf(newPrefix,"%s/%s", prefix, component); 

		expandWildcards(newPrefix, arg);
		return;
	}

	//Convert component to regEx
	char * reg = (char*)malloc(2*strlen(arg)+10);
	char * a = component;
	char * r = reg;
	int i = 0;
	int j = 0;
	r[j] = '^'; 
	j++;
	while (a[i] != '\0'){
		if (a[i] == '*'){
			r[j] = '.';
			j++;
			r[j] = '*';
			j++;
		} else if (a[i] == '?'){
			r[j] = '.';
			j++;
		} else if (a[i] == '.'){
			r[j] = '\\';
			j++;
			r[j] = '.';
			j++;
		} else {
			r[j] = a[i];
			j++;
		}
		i++;
	}
	r[j] = '$'; j++; r[j] = 0; //close end of expanded string
	//fprintf(stdout, "%s\n", reg);

	//compile regex
	regex_t re;
	int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);
	if (expbuf != 0){
		perror("compile\n");
		return;
	}

	//Directory portion
	char * dir;	
	if (prefix == NULL || prefix[0] == '\0'){
		dir = (char*)".";
	} else dir = prefix;

	DIR * d = opendir(dir);
	if (d == NULL){
		//perror("opendir");
		return;
	}

	//collect matching entries
	struct dirent * ent;
	int maxEnts = 20;
	int nEnts = 0;
	std::vector<char*> eArr;
	regmatch_t match;
	while((ent = readdir(d))!= NULL){
		if (regexec(&re, ent->d_name, 1, &match, 0) ==0 ) {
			if (ent->d_name[0] == '.'){
				if (arg[0] == '.'){
					eArr.push_back(strdup(ent->d_name));
					nEnts++;
				}
			} else {
				eArr.push_back(strdup(ent->d_name));
				nEnts++;
				//sprintf(newPrefix,"%s%s", prefix, ent->d_name); 
				//expandWildcards(newPrefix,arg);
			}
		}
	}
	closedir(d);

	//Sort entries
	std::sort(eArr.begin(), eArr.begin() + nEnts, comp);
	//Add entries to _arguments
	for (i = 0; i < nEnts; i++ ){
		//printf("(%s, %s)", eArr[i], arg);
		if(prefix == NULL || prefix == '\0') prefix = (char*)"";
		if (arg[0] == 0){
			sprintf(newPrefix,"%s%s", prefix, eArr[i]); 
		} else {
			sprintf(newPrefix,"%s%s/", prefix, eArr[i]); 
		}
		//sprintf(newPrefix,"%s%s", prefix, eArr[i]); 
		expandWildcards(newPrefix,arg);
		//		Command::_currentSimpleCommand->
		//			insertArgument(eArr[i]);
	}
}

//Comparison function for sorting directory entries (Wildcard)
bool comp (char* i,char* j) { 
	return strcmp(i, j) < 0; 
}


%}

%%

goal:	
commands
;

commands: 
command
| commands command 
;

command: simple_command
;

simple_command:	
pipe_list iomodlist background_opt NEWLINE {
	//printf("   Yacc: Execute command\n");
	Command::_currentCommand.execute();
}
| NEWLINE 
| error NEWLINE { yyerrok; }
;

command_and_args:
command_word argument_list {
	//printf("commandAndArgsRule\n");
	Command::_currentCommand.
		insertSimpleCommand( Command::_currentSimpleCommand );
}
;

argument_list:
argument_list argument
| /* can be empty */
;

argument:
WORD {
	// printf("   Yacc: insert argument \"%s\"\n", $1);
	//Command::_currentSimpleCommand->insertArgument( $1 );
	expandWildcards('\0',$1);	
}
;
pipe_list:
command_and_args {
}
|
pipe_list PIPE command_and_args {
}
;

command_word:
WORD {
	// printf("   Yacc: insert command \"%s\"\n", $1); 
	Command::_currentSimpleCommand = new SimpleCommand();
	Command::_currentSimpleCommand->insertArgument( $1 );
}
|
EXIT {
	//Command::_currentSimpleCommand = new SimpleCommand();
	Command::_currentSimpleCommand->killme();
}
;

iomodifier_opt:
GREAT WORD {
	//printf("   Yacc: insert output \"%s\"\n", $2);
	if (Command::_currentCommand._outFile != 0) {
		printf("Ambiguous output redirect\n");
	} else {
		Command::_currentCommand._outFile = $2;
	}
}
|
GREATGREAT WORD{	
	//printf("   Yacc: insert output \"%s\"\n", $2);
	Command::_currentCommand._outFile = $2;
	Command::_currentCommand._append = 1;
}
|
GREATGREATAMP WORD{
	//printf("   Yacc: insert output \"%s\"\n", $2);
	Command::_currentCommand._outFile = $2;
	Command::_currentCommand._errFile = $2;
	Command::_currentCommand._append = 1;
}
|
GREATAMP WORD {
	//printf("   Yacc: insert output \"%s\"\n", $2);
	Command::_currentCommand._errFile = $2;
	Command::_currentCommand._outFile = $2;
}
|
LESS WORD{
	//printf("   Yacc: insert output \"%s\"\n", $2);
	Command::_currentCommand._inFile = $2;
}
;

iomodlist:
iomodlist iomodifier_opt 
|
;

background_opt:
AMP {
	//printf("fixstuff?maybe?\n");
	Command::_currentCommand._background = 1;
}
|
;

%%
	void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
