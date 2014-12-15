#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <assert.h>

#include "ast.h"
#include "symbol.h"
#include "common.h"
#include "parser.tab.h"

#define DEBUG_PRINT_TREE 0

node *ast = NULL;

node *ast_allocate(node_kind kind, ...) {
  va_list args;

  // make the node
  node *ast = (node *) malloc(sizeof(node));
  memset(ast, 0, sizeof *ast);
  ast->kind = kind;
  ast->st = st_curr;

  va_start(args, kind); 

  switch(kind) {

  case DECLARATIONS_NODE:
    ast->declarations.declarations = va_arg(args, node *);
    ast->declarations.declaration = va_arg(args, node *);
    break;

  case DECLARATION_NODE:
    ast->declaration.var_name = strdup(va_arg(args, char *));
    ast->declaration.init_val = va_arg(args, node *);
    break;

  case STATEMENTS_NODE:
    ast->statements.statements = va_arg(args, node *);
    ast->statements.statement = va_arg(args, node *);
    break;

  /* Start statement nodes */
  case ASSIGNMENT_NODE:
    ast->assign_stmt.var = va_arg(args, node *);
    ast->assign_stmt.new_val = va_arg(args, node *);
    break;
 
  case IF_STATEMENT_NODE:
    ast->if_stmt.expr = va_arg(args, node *);
    ast->if_stmt.stmt = va_arg(args, node *);
    ast->if_stmt.opt_stmt = va_arg(args, node *);  
  break;
 
  case SCOPE_NODE:
    ast->scope.declarations = va_arg(args, node *);
    ast->scope.statements = va_arg(args, node *);
    break;
  /* End statement nodes */

  /* Start expression nodes */
  case UNARY_EXPRESSION_NODE:
    ast->unary_expr.op = va_arg(args, int);
    ast->unary_expr.expr = va_arg(args, node *);
    break;

  case BINARY_EXPRESSION_NODE:
    ast->binary_expr.op = va_arg(args, int);
    ast->binary_expr.left = va_arg(args, node *);
    ast->binary_expr.right = va_arg(args, node *);
    break;

  case BOOL_NODE:
    ast->bool_lit.value = va_arg(args, int);
    break;

  case INT_NODE:
    ast->int_lit.value = va_arg(args, int);
    break;

  case FLOAT_NODE:
    ast->float_lit.value = va_arg(args, double);
    break;

  case VAR_NODE:
    ast->var.name = strdup(va_arg(args, char *));
    ast->var.ofs = va_arg(args, int);
    break;

  case FUNCTION_NODE:
	ast->function.func = (func_t)va_arg(args, int);
	ast->function.args_opt = va_arg(args, node *);
	break;

  case CONSTRUCTOR_NODE:
	ast->constructor.type = (type_t)va_arg(args, int);
	ast->constructor.args_opt = va_arg(args, node *);
	break;
  /* End expression nodes */

  case ARGUMENTS_NODE:
    ast->arguments.args = va_arg(args, node *);
    ast->arguments.expr = va_arg(args, node *);
    break;

  default:
    fprintf(outputFile, "ast_allocate: Unsupported node kind.\n"); 
    break;
  }

  va_end(args);

  return ast;
}

void ast_free(node *ast) {
	switch(ast->kind){
			case SCOPE_NODE:
				if(ast->scope.declarations != NULL)
					ast_free(ast->scope.declarations); 
				if(ast->scope.statements != NULL)
					ast_free(ast->scope.statements);
				free(ast);
				break;
			case DECLARATIONS_NODE:
				if(ast->declarations.declarations != NULL)
					ast_free(ast->declarations.declarations); 
				ast_free(ast->declarations.declaration);
				free(ast);
				break;
			case STATEMENTS_NODE:
				if(ast->statements.statements != NULL)
					ast_free(ast->statements.statements);
				ast_free(ast->statements.statement);
				free(ast);
				break;
			case DECLARATION_NODE:
				if(ast->declaration.init_val != NULL)
					ast_free(ast->declaration.init_val);
				free(ast);
				break;
			case ASSIGNMENT_NODE:
				ast_free(ast->assign_stmt.var);
				ast_free(ast->assign_stmt.new_val);
				free(ast);
				break;
			case UNARY_EXPRESSION_NODE:
				ast_free(ast->unary_expr.expr);
				free(ast);
				break;
			case BINARY_EXPRESSION_NODE:
				ast_free(ast->binary_expr.left);
				ast_free(ast->binary_expr.right);
				free(ast);
				break;
			case BOOL_NODE:
				free(ast); //this works since there's no AST nodes, assuming free will free all non node members
				break;
			case INT_NODE:
				free(ast);
				break;
			case FLOAT_NODE:
				free(ast);
				break;
			case VAR_NODE:
				free(ast); //double check this
				break;
			case FUNCTION_NODE:
				if(ast->function.args_opt != NULL)				
					ast_free(ast->function.args_opt);
				free(ast);
				break;
			case CONSTRUCTOR_NODE:
				if(ast->constructor.args_opt != NULL)
					ast_free(ast->constructor.args_opt);
				free(ast);
				break;
			case IF_STATEMENT_NODE:
				ast_free(ast->if_stmt.expr);
				ast_free(ast->if_stmt.stmt);
				if(ast->if_stmt.opt_stmt != NULL)
					ast_free(ast->if_stmt.opt_stmt);
				free(ast);
				break;
			case ARGUMENTS_NODE:
				if(ast->arguments.args != NULL)				
					ast_free(ast->arguments.args);
				ast_free(ast->arguments.expr);
				free(ast);
				break;
			default:
				fprintf(outputFile, "ast_free: Warning: Unexpected node kind.\n");
				break;
	}
}

/* These two flags are used to print things properly */
static int dclns_flag = 0;
static int stmts_flag = 0;

int print_type_index(type_t type){
	if(type == INT) return 0;
	if(type == IVEC2) return 1;
	if(type == IVEC3) return 2;
	if(type == IVEC4) return 3;
	if(type == FLOAT) return 4;
        if(type == VEC2) return 5;
        if(type == VEC3) return 6;
        if(type == VEC4) return 7;
        if(type == BOOL) return 8;
        if(type == BVEC2) return 9;
        if(type == BVEC3) return 10;
        if(type == BVEC4) return 11;
	return 12;
}

const char * type_strings[NUM_TYPES] = {
        "int",
        "ivec2",
        "ivec3",
        "ivec4",
        "float",
        "vec2",
        "vec3",
        "vec4",
        "bool",
        "bvec2",
        "bvec3",
        "bvec4",
        "any"
};

int print_bop_index(int op){
	if(op == _AND) return 0;
	if(op == _OR) return 1;
	if(op == _NEQ) return 2;
	if(op == _EQ) return 3;
	if(op == _LEQ) return 4;
	if(op == _GEQ) return 5;
	if(op == '<') return 6;
	if(op == '>') return 7;
	if(op == '+') return 8;
	if(op == '-') return 9;
	if(op == '*') return 10;
	if(op == '/') return 11;
	if(op == '^') return 12;
	else return 13;
}

static const char * bop_strings[NUM_OPS + 1] = {
	"AND",
	"OR",
	"NEQ",
	"EQ",
	"LEQ",
	"GEQ",
	"<",
	">",
	"+",
	"-",
	"*",
	"/",
	"^",
	"garbage op"
};

int print_func_index(func_t func){
	if(func == DP3) return 0;
	if(func == LIT) return 1;
	if(func == RSQ) return 2;
	else return 3;
}

const char * func_strings[NUM_FUNCS + 1] = {
	"dp3",
	"lit",
	"rsq",
	"garbage func"
};

// forward declare for ast_print_args
static void ast_print_expr(node *);

static void ast_print_args(node *ast){

	assert(ast != NULL);
	assert(ast->kind & ARGUMENTS_NODE);

	if(ast->arguments.args != NULL)
		ast_print_args(ast->arguments.args);
	ast_print_expr(ast->arguments.expr);

	return;
}

static void ast_print_expr(node *ast){

	assert(ast != NULL);
	assert(ast->kind & EXPRESSION_NODE);

	switch(ast->kind){
	  case UNARY_EXPRESSION_NODE:
		fprintf(outputFile, "UNARY\n");
		fprintf(outputFile, "type: %s\n", type_strings[print_type_index(ast->unary_expr.type)]);
		fprintf(outputFile, "op: %c\n", (char)ast->unary_expr.op);
		ast_print_expr(ast->unary_expr.expr);
		break;
	  case BINARY_EXPRESSION_NODE:
		fprintf(outputFile, "BINARY\n");
		fprintf(outputFile, "type: %s\n", type_strings[print_type_index(ast->binary_expr.type)]);
		fprintf(outputFile, "op: %s\n", bop_strings[print_bop_index(ast->binary_expr.op)]);
		ast_print_expr(ast->binary_expr.left);
		ast_print_expr(ast->binary_expr.right);
		break;
	  case BOOL_NODE:
                fprintf(outputFile, "%s\n", ast->bool_lit.value ? "true" : "false");
                break;
	  case INT_NODE:
		fprintf(outputFile, "%d\n", ast->int_lit.value);
		break;
	  case FLOAT_NODE:
                fprintf(outputFile, "%f\n", ast->float_lit.value);
                break;
	  case VAR_NODE:
		if(ast->var.ofs == -1)
			fprintf(outputFile, "%s\n", ast->var.name);
		else fprintf(outputFile, "%s[%d]\n", ast->var.name, ast->var.ofs);
		break;
	  case FUNCTION_NODE:
		fprintf(outputFile, "CALL\n");
		fprintf(outputFile, "function name: %s\n", func_strings[print_func_index(ast->function.func)]);
		if(ast->function.args_opt != NULL)
			ast_print_args(ast->function.args_opt);
		break;
	  case CONSTRUCTOR_NODE:
		fprintf(outputFile, "CALL\n");
		fprintf(outputFile, "constructor type: %s\n", type_strings[print_type_index(ast->constructor.type)]);
		if(ast->constructor.args_opt != NULL)
			ast_print_args(ast->constructor.args_opt);
		break;
	  default:
		fprintf(outputFile, "ast_print_expr: Unsupported expression type.\n");
		break;
	}
	
	return;
}

/* forward declare for ast_print_stmt */
static void ast_print_dclns(node *);
static void ast_print_stmts(node *);

static void ast_print_stmt(node *ast){
	struct st_entry *ste;
	
	if(ast == NULL) return;
	assert(ast->kind & STATEMENT_NODE);

	switch(ast->kind){
          case ASSIGNMENT_NODE:
                fprintf(outputFile, "ASSIGN\n");
		ste = st_lookup(ast->st, ast->assign_stmt.var->var.name, GLOBAL);
		if(ste == NULL){
			/* Variable undeclared */
			fprintf(outputFile, "type: any\n");
		} else fprintf(outputFile, "type: %s\n", type_strings[print_type_index(ste->type)]);
		fprintf(outputFile, "var_name: ");
		if(ast->assign_stmt.var->var.ofs == -1)
			fprintf(outputFile, "%s\n", ast->assign_stmt.var->var.name);
		else fprintf(outputFile, "%s[%d]\n", ast->assign_stmt.var->var.name, ast->assign_stmt.var->var.ofs);
		ast_print_expr(ast->assign_stmt.new_val);
                break;
	  case IF_STATEMENT_NODE:
		fprintf(outputFile, "IF\n");
		ast_print_expr(ast->if_stmt.expr);
		ast_print_stmt(ast->if_stmt.stmt);
		if(ast->if_stmt.opt_stmt != NULL)
			ast_print_stmt(ast->if_stmt.opt_stmt);
		break;
	  case SCOPE_NODE:
		fprintf(outputFile, "SCOPE\n");
		/* Reset print flags */
		dclns_flag = 0;
		stmts_flag = 0;
		ast_print_dclns(ast->scope.declarations);
		ast_print_stmts(ast->scope.statements);
		fprintf(outputFile, "END SCOPE\n");
		break;
          default:
                fprintf(outputFile, "ast_print_expr: Unsupported statement type.\n");
                break;
        }

	return;
}

static void ast_print_dcln(node *ast){
	struct st_entry *ste;	

	assert(ast != NULL);
	assert(ast->kind == DECLARATION_NODE);
	fprintf(outputFile, "DECLARATION\n");
	
	fprintf(outputFile, "var_name: %s\n", ast->declaration.var_name);
	ste = st_lookup(ast->st, ast->declaration.var_name, GLOBAL);
	if(ste == NULL){
		/* Variable undeclared - this should not happen */
		fprintf(outputFile, "type_name: any (ERROR)\n"); 
	} else fprintf(outputFile, "type_name: %s\n", type_strings[print_type_index(ste->type)]);
	if(ast->declaration.init_val != NULL){
		fprintf(outputFile, "init_val: \n");
		ast_print_expr(ast->declaration.init_val);
	}

	return;
}

static void ast_print_stmts(node *ast){
	
	if(stmts_flag == 0){
		fprintf(outputFile, "STATEMENTS\n");
		stmts_flag = 1;
	}

	if(ast == NULL) return;
	assert(ast->kind == STATEMENTS_NODE);

	ast_print_stmts(ast->statements.statements);
	ast_print_stmt(ast->statements.statement);

	return;
}

static void ast_print_dclns(node *ast){
	
	if(dclns_flag == 0){
		fprintf(outputFile, "DECLARATIONS\n");
		dclns_flag = 1;
	}
	
	if(ast == NULL) return;
	assert(ast->kind == DECLARATIONS_NODE);	

	ast_print_dclns(ast->declarations.declarations);
	ast_print_dcln(ast->declarations.declaration);
	
	return;
}

/* Print to stdout for now */
void ast_print(node *ast) {

	assert(ast != NULL);
	assert(ast->kind == SCOPE_NODE);
	
	/* Expecting a scope, which is a statement */
	ast_print_stmt(ast);

	return;
}
