
/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <string>
#include <signal.h>
#include <fcntl.h>
#include <regex.h>
#include "command.h"

void zombiekiller(int signum);

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numOfAvailableArguments = 5;
	_numOfArguments = 0;
	_arguments = (char **) malloc( _numOfAvailableArguments * sizeof( char * ) );
}

void SimpleCommand:: killme(){
	printf("Bye!\n");
	exit(0);
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numOfAvailableArguments == _numOfArguments  + 1 ) {
		// Double the available space
		_numOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numOfAvailableArguments * sizeof( char * ) );
	}
	//Perform VarExpansion check here:	
	const char * regEx = "^.*${[^}][^}]*}.*$";
	regex_t re;
	int result = regcomp( &re, regEx, 0);
	regmatch_t match;
	result = regexec( &re, argument, 1, &match, 0 );
	//printf("Result:%d\n", result);
	if ( result == 0 ) { // matches regEx
		//parse through and get "var"
		//char * buffer = (char*) "^.*${[^}][^}]*}.*$";
		//char * expbuf = compile(buffer, 0, 0);
		int slocation = -1;
		int elocation = -1;
		int x = 0;
		char* replacementArg;
		std::string bufferArg;
		std::string tempArg;

		while(argument[x] != '\0'){
			if (argument[x] == '$') slocation = x;
			if (argument[x] == '}') elocation = x;
			if (slocation != -1 && elocation != -1){	
				std::string repString = argument;
				repString = repString.substr(slocation + 2, elocation - slocation - 2);
				tempArg = getenv(repString.c_str());
				bufferArg = bufferArg + tempArg;
				slocation = -1;
				elocation = -1;
			} else if (slocation == -1 && elocation == -1){
				bufferArg = bufferArg + argument[x];
			}
			x++;
		}
		replacementArg = (char*)(malloc(sizeof(char)*x));
		strcpy(replacementArg, bufferArg.c_str());
		_arguments[ _numOfArguments ] = replacementArg;
	} else {
		_arguments[ _numOfArguments ] = argument;
	}

	_arguments[ _numOfArguments + 1] = NULL;

	_numOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
}

	void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
				_numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}

	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

	void
Command:: clear()
{
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}

		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}
	int dupliCatch = 0;
	if (_outFile == _errFile){
		dupliCatch = 1;
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inFile ) {
		free( _inFile );
	}

	if ( _errFile && dupliCatch == 0 ) {
		free( _errFile );
	}

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;
}

	void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
			_inFile?_inFile:"default", _errFile?_errFile:"default",
			_background?"YES":"NO");
	printf( "\n\n" );

}

	void
Command::execute()
{

	int tempIn = dup(0);
	int tempOut = dup(1);
	int tempErr = dup(2);
	int fdin;
	int fderr;
	if (_inFile != NULL){
		fdin = open(_inFile, O_RDWR);
	} else {
		fdin = dup(tempIn);
	}

	if (_errFile != NULL){
		if (_append == 1){
			fderr = open(_errFile, O_RDWR | O_APPEND);
		} else {
			fderr = open(_errFile, O_RDWR | O_CREAT, S_IRWXU);
		}
	} else {
		fderr = dup(tempErr);
	}

	// Add execution here
	int returner;
	int fdout;
	for (int i = 0; i < _numOfSimpleCommands; i++){
		dup2(fdin, 0);
		close(fdin);	
		dup2(fderr, 2);
		if (i == _numOfSimpleCommands - 1){
			if (_outFile){
				if (_append == 1){
					fdout = open(_outFile, O_APPEND | O_RDWR);
				} else {
					fdout = open(_outFile, O_CREAT | O_RDWR, S_IRWXU);
				}
			} else {
				fdout = dup(tempOut);
			}

		} else  {
			// Not last
			//simple command  
			//create pipe  
			int  pipefd[2];  
			pipe (pipefd);  
			fdout = pipefd[1];  
			fdin = pipefd[0];
		}

		//Redirect output
		dup2(fdout, 1);
		close(fdout);
		close(fderr);
		//Create child processes
		//setenv check
		if ( !strcmp( _simpleCommands[i]->_arguments[0], "setenv" ) ){
			setenv(_simpleCommands[i]->_arguments[1], 
					_simpleCommands[i]->_arguments[2], 1);
		} else if (!strcmp( _simpleCommands[i]->_arguments[0], "unsetenv")){
			unsetenv(_simpleCommands[i]->_arguments[1]);
		} else if (!strcmp( _simpleCommands[i]->_arguments[0], "cd")){
			char * addr = _simpleCommands[i]->_arguments[1];
			if (addr == NULL) addr = (char*)getenv("HOME");
			int c = chdir(addr);
			if (c != 0) perror("chdir");
		} else {
			_waiting = 1;
			returner = fork();
			if (returner == 0){
				if (!strcmp(_simpleCommands[i]->_arguments[0], "printenv")) {
					extern char ** environ;
					int x;
					for(x = 0; environ[x]; x++){
						printf("%s\n", environ[x]);
					}
				} else {
					execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
					//	perror("execvp");
				}
				exit(0);
			} else if (returner < 0){
				perror("Fork");
				return;
			}
		}
	}
	//restore in/out defaults
	dup2(tempIn, 0);
	dup2(tempOut, 1);
	dup2(tempErr, 2);
	close(tempIn);
	close(tempOut);
	close(tempErr);

	if (!_background){
		waitpid(returner, NULL, 0);
		_waiting = 0;
	}
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec

	// Clear to prepare for next command
	clear();

	// Print new prompt
	if (isatty(0)) prompt();
}

// Shell implementation

	void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void sigHandler(int signum){
	printf("\n");
	Command::_currentCommand.clear();
	if (isatty(0) && !Command::_currentCommand._waiting){	
		Command::_currentCommand.prompt();
	}
}

void zombiekiller(int signum){
	//printf("Killing Zombies\n");
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


main()
{
	if (isatty(0)){	
		Command::_currentCommand.prompt();
		//Command::printedPrompt = 1;
	}

	signal(SIGINT, sigHandler);
	signal(SIGCHLD, zombiekiller);	
	//ZombieCode

	//	char s[ 20 ];
	//	fgets( s, 20, stdin );
	//
	//	if ( !strcmp( s, "exit\n" ) ) {
	//		printf( "Bye!\n");
	//		exit( 1 );
	//	}
	yyparse();
}


