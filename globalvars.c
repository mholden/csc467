/***********************************************************************
 * **YOUR GROUP INFO SHOULD GO HERE**
 *
 * globalvars.c
 *
 * CSC467 Project Compiler Shared Global Variables
 *
 * This file contains the definition of global variables that are used
 * for communication among the various compiler modules
 **********************************************************************/

#include <stdio.h>
#include "symbol.h"

/***********************************************************************
 * FILE global variables.
 * Used to specify sinks for compiler output and sources for compiler
 * input.
 **********************************************************************/
FILE * inputFile;
FILE * outputFile;
FILE * errorFile;
FILE * dumpFile;
FILE * traceFile;
FILE * runInputFile;
FILE * assemblyFile;

/***********************************************************************
 * Control flags, set by main.c, used to cause various optional compiler
 * actions to take place. 
 **********************************************************************/
int errorOccurred;
int suppressExecution;

int traceScanner;
int traceParser;
int traceExecution;

int dumpSource;
int dumpAST;
int dumpSymbols;
int dumpInstructions;

/***********************************************************************
 * Scanner/Parser/AST/Semantics global variables.
 *
 * **NOTE** If you need to add global variables for phases 1 to 4, add
 * them below this comment.
 **********************************************************************/

symbol_table_t *st_curr = NULL;



