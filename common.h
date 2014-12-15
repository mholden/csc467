/***********************************************************************
 * **YOUR GROUP INFO SHOULD GO HERE**
 *
 * common.h
 *
 * common definitions that are used throughout the compiler.
 **********************************************************************/

#ifndef _COMMON_H_ 
#define _COMMON_H_ 

#include <stdio.h> /* for FILE */
 
/**********************************************************************
 * Some useful definitions. These may be modified or removed as needed.
 *********************************************************************/ 
#ifndef TRUE
# define TRUE  1
#endif
#ifndef FALSE
# define FALSE 0
#endif
 
#define MAX_IDENTIFIER 32
#define MAX_TEXT       256
#define MAX_INTEGER    32767
#define NUM_TYPES      13
#define NUM_FUNCS	3
#define NUM_OPS		13
/********************************************************************** 
 * External declarations for variables declared in globalvars.c.
 **********************************************************************/
extern FILE * inputFile;
extern FILE * outputFile;
extern FILE * errorFile;
extern FILE * dumpFile;
extern FILE * traceFile;
extern FILE * runInputFile;
extern FILE * assemblyFile;

extern int errorOccurred;
extern int suppressExecution;

extern int traceScanner;
extern int traceParser;
extern int traceExecution;

extern int dumpSource;
extern int dumpAST;
extern int dumpSymbols;
extern int dumpInstructions;

typedef struct symbol_table symbol_table_t;
extern symbol_table_t *st_curr;

typedef enum {
  INT		= (1 << 3), 
  IVEC2		= (1 << 3) | (1 << 0),
  IVEC3		= (1 << 3) | (1 << 1),
  IVEC4		= (1 << 3) | (1 << 2),
  FLOAT		= (1 << 7),
  VEC2		= (1 << 7) | (1 << 4),
  VEC3		= (1 << 7) | (1 << 5),
  VEC4		= (1 << 7) | (1 << 6),
  BOOL		= (1 << 11),
  BVEC2		= (1 << 11) | (1 << 8),
  BVEC3		= (1 << 11) | (1 << 9),
  BVEC4		= (1 << 11) | (1 << 10),
  ANY		= (1 << 11) | (1 << 7) | (1 << 3)
} type_t;

int print_type_index(type_t type);

extern const char *type_strings[NUM_TYPES];

enum {
  _AND = 0,
  _OR,
  _NEQ,
  _EQ,
  _LEQ,
  _GEQ
};

typedef enum {
        LOCAL = 0,
        GLOBAL
} scope_t;

typedef enum {
  DP3 = 0, 
  LIT = 1, 
  RSQ = 2
} func_t;

int print_func_index(func_t func);

extern const char *func_strings[NUM_FUNCS + 1];

int print_bop_index(int op);
#endif

