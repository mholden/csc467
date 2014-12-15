
#ifndef AST_H_
#define AST_H_ 1

#include <stdarg.h>
#include "symbol.h"
#include "common.h"

// Dummy node just so everything compiles, create your own node/nodes
//
// The code provided below is an example ONLY. You can use/modify it,
// but do not assume that it is correct or complete.
//
// There are many ways of making AST nodes. The approach below is an example
// of a descriminated union. If you choose to use C++, then I suggest looking
// into inheritance.

#define MAX_TYPE_LEN 10

// forward declare
struct node_;
typedef struct node_ node;
extern node *ast;

typedef enum {
  UNKNOWN               = 0,

  EXPRESSION_NODE       = (1 << 2),
  UNARY_EXPRESSION_NODE = (1 << 2) | (1 << 3),
  BINARY_EXPRESSION_NODE= (1 << 2) | (1 << 4),
  BOOL_NODE		= (1 << 2) | (1 << 5),
  INT_NODE              = (1 << 2) | (1 << 6), 
  FLOAT_NODE            = (1 << 2) | (1 << 7),
  IDENT_NODE            = (1 << 2) | (1 << 8),
  VAR_NODE              = (1 << 2) | (1 << 9),
  FUNCTION_NODE         = (1 << 2) | (1 << 10),
  CONSTRUCTOR_NODE      = (1 << 2) | (1 << 11),

  STATEMENT_NODE        = (1 << 1),
  IF_STATEMENT_NODE     = (1 << 1) | (1 << 12),
  ASSIGNMENT_NODE       = (1 << 1) | (1 << 13),
  SCOPE_NODE		= (1 << 1) | (1 << 14),  

  STATEMENTS_NODE	= (1 << 1) | (1 << 15),

  DECLARATION_NODE      = (1 << 16),
  DECLARATIONS_NODE	= (1 << 17),
  
  ARGUMENTS_NODE 	= (1 << 18)
} node_kind;

struct node_ {

  // an example of tagging each node with a type
  node_kind kind;
  
  /* A pointer to our symbol table */
  symbol_table_t *st;

  union {
    struct {
	node *declarations;
	node *declaration;
    } declarations;

    struct {
        char *var_name;
	node *init_val;
    } declaration;

    struct {
        node *statements;
        node *statement;
    } statements;

    /* Statement nodes */
    struct {
    	node *var;
	node *new_val;
    } assign_stmt;

    struct {
        node *expr;
        node *stmt;
	node *opt_stmt;
    } if_stmt;

    struct {
        node *declarations;
        node *statements;
    } scope;
    /* End statement nodes */
  
    /* Expression nodes - all these should have types  */
    struct {
      	int op;
      	node *expr;
	type_t type;
    } unary_expr;

    struct {
     	int op;
      	node *left;
      	node *right;
	type_t type;
    } binary_expr;

    struct {
        int value;
        type_t type;
    } bool_lit;

    struct {
        int value;
	type_t type;
    } int_lit;

    struct {
        double value;
	type_t type;
    } float_lit;

    struct {
        char *name;
	int ofs;
	type_t type;
    } var;

    struct {
	func_t func;
	node *args_opt;
    } function;

    struct {
	type_t type;
	node *args_opt;
    } constructor;
    /* End expression nodes */

    struct {
	node *args;
	node *expr;
    } arguments;

    // etc.
  };
};

node *ast_allocate(node_kind type, ...);
void ast_free(node *ast);
void ast_print(node * ast);

#endif /* AST_H_ */
