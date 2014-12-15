#include "codegen.h"
#include "common.h"
#include "symbol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF_LEN (MAX_IDENTIFIER + 4)

#define NUM_MAPPED_REGS 13
static const char *mapped_vars[NUM_MAPPED_REGS] = {
	"gl_FragColor",
	"gl_FragDepth",
	"gl_FragCoord",
	"gl_TexCoord",
	"gl_Color",
	"gl_Secondary",
	"gl_FogFragCoord",
	"gl_Light_Half",
	"gl_Light_Ambient",
	"gl_Material_Shininess",
	"env1",
	"env2",
	"env3"
};

static const char *mapped_regs[NUM_MAPPED_REGS] = {
        "result.color",
        "result.depth",
        "fragment.position",
        "fragment.texcoord",
        "fragment.color",
        "fragment.color.secondary",
        "fragment.fogcoord",
        "state.light[0].half",
        "state.lightmodel.ambient",
        "state.material.shininess",
        "program.env[1]",
        "program.env[2]",
        "program.env[3]"
};

static const char *zero_reg = "zero_reg";
static const char *true_reg = "true_reg";
static const char *false_reg = "false_reg";

static void var_to_assembly(char *assembly, char *varname, int index){
	int i;

	for(i = 0; i < NUM_MAPPED_REGS; i++){
		if(!strcmp(varname, mapped_vars[i]))
			break;
	}

	if(i == NUM_MAPPED_REGS){
		/* Not one of the mapped vars, just use varname */
		strncpy(assembly, varname, MAX_BUF_LEN);
	} else {
		/* One of the mapped vars, use the mapped reg */
		strncpy(assembly, mapped_regs[i], MAX_BUF_LEN);
	}

	/* Deal with offset, if there is one */
	if(index != -1){
		strcat(assembly, ".");
		switch(index){
			case 0:
				strcat(assembly, "x");
				break;
			case 1:
				strcat(assembly, "y");
				break;
			case 2:
				strcat(assembly, "z");
				break;
			default:
				strcat(assembly, "w");
				break;
		}
	}

	return;
}

#define MAX_TEMP_REGS 20

struct trt_entry{
	char regname[MAX_BUF_LEN];
	bool curr_used;
	bool ever_used;
};

struct tempreg_table{
	struct trt_entry entries[MAX_TEMP_REGS];
};

static struct tempreg_table trt;

static void free_tempreg(char *source){
	int i;

	if(strstr(source, "tempVar")){
		sscanf(source + 7 /* tempVar* */, "%d", &i);
		trt.entries[i].curr_used = FALSE;
	}

	return;
}

static void get_tempreg(char *dest){
	int i;

	for(i = 0; i < MAX_TEMP_REGS; i++){
		if(trt.entries[i].curr_used == FALSE)
			break;
	}

	if(i == MAX_TEMP_REGS){
		fprintf(errorFile, "get_tempreg: Error: out of regs!\n");
		return;
	}
	else{
		if(trt.entries[i].ever_used == FALSE){
			fprintf(assemblyFile, "TEMP\t%s;\n", trt.entries[i].regname);
			trt.entries[i].ever_used = TRUE;
		}
		strncpy(dest, trt.entries[i].regname, MAX_BUF_LEN);
		trt.entries[i].curr_used = TRUE;
	}
	
	return;
}

static void init_tempregs(){
	int i;

	for(i = 0; i < MAX_TEMP_REGS; i++){
		sprintf(trt.entries[i].regname, "tempVar%d", i);
		trt.entries[i].curr_used = FALSE;
		trt.entries[i].ever_used = FALSE;
	}
}

static void init_utilregs(){
        fprintf(assemblyFile, "PARAM\t%s = 0.0;\n", zero_reg);
        fprintf(assemblyFile, "PARAM\t%s = 1.0;\n", true_reg);
        fprintf(assemblyFile, "PARAM\t%s = -1.0;\n", false_reg);
}

/* Foward declaration for genCode_args */
static void genCode_expr(node *ast, char *result);

static void genCode_args(node *ast, int *arg_count, char *arg0, char *arg1, char *arg2, char *arg3){
	
	if(ast->arguments.args != NULL){
		genCode_args(ast->arguments.args, arg_count, arg0, arg1, arg2, arg3);
	}

	switch(*arg_count){
		case 0:
			genCode_expr(ast->arguments.expr, arg0);
			break;
		case 1:
			genCode_expr(ast->arguments.expr, arg1);
			break;
		case 2:
			genCode_expr(ast->arguments.expr, arg2);
			break;
		case 3:
			genCode_expr(ast->arguments.expr, arg3);
			break;
		default:
			break;
	}
	
	(*arg_count)++;

	return;
}

static void genCode_expr(node *ast, char *result){
	char buf1[MAX_BUF_LEN], buf2[MAX_BUF_LEN], buf3[MAX_BUF_LEN], buf4[MAX_BUF_LEN];
	char dest[MAX_BUF_LEN], value[MAX_BUF_LEN];
	int arg_count;

	switch(ast->kind){
		case UNARY_EXPRESSION_NODE:
			genCode_expr(ast->unary_expr.expr, buf1);
                       	get_tempreg(dest);

			switch(ast->unary_expr.op){
				case '!':
					fprintf(assemblyFile, "# unary !:\n");
					fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, buf1, true_reg, false_reg);
					break;
				case '-':
					fprintf(assemblyFile, "# unary -:\n");
					fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, buf1);
					break;
				default:
					strncpy(dest, "genCode_expr: Error: Unimplemented.", MAX_BUF_LEN);
					break;
			}
		
			free_tempreg(buf1);
                       	strncpy(result, dest, MAX_BUF_LEN);

			break;
		case BINARY_EXPRESSION_NODE:
			genCode_expr(ast->binary_expr.left, buf1);
			genCode_expr(ast->binary_expr.right, buf2);
			get_tempreg(dest);

			switch(ast->binary_expr.op){
				case _AND:
					fprintf(assemblyFile, "# binary AND:\n");
					fprintf(assemblyFile, "ADD\t%s, %s, %s;\n", dest, buf1, buf2);
					/* If both true, dest == 2. Else dest == 0 or -2 */
					fprintf(assemblyFile, "SGE\t%s, %s, %s;\n", dest, dest, true_reg);
					/* dest == 1 (true) or 0 (false) */
					fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
					/* dest == -1 (true) or 0 (false) */
					fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
					/* dest == 1 or -1, finally, which is what we want. */
					break;
				case _OR:
					fprintf(assemblyFile, "# binary OR:\n");
					fprintf(assemblyFile, "ADD\t%s, %s, %s;\n", dest, buf1, buf2);
                                        /* If either true, dest >= 0. Else dest -2 */
                                        fprintf(assemblyFile, "SGE\t%s, %s, %s;\n", dest, dest, zero_reg);
                                        /* dest == 1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
                                        /* dest == -1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
                                        /* dest == 1 (true) or -1 (false) */
                                        break;
				case _EQ:
					fprintf(assemblyFile, "# binary EQ:\n");
					fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, buf1, buf2);
					/* dest == 0 if buf1 == buf2 */
					fprintf(assemblyFile, "ABS\t%s, %s;\n", dest, dest);
					/* dest > 0 if buf1 != buf2 */
					fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
					/* dest < 0 if buf1 != buf2 */
					fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, false_reg, true_reg);
					/* dest == 1 (true) or -1 (false) */
                                        break;
				case _NEQ:
					fprintf(assemblyFile, "# binary NEQ:\n");
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, buf1, buf2);
                                        /* dest == 0 if buf1 == buf2 */
                                        fprintf(assemblyFile, "ABS\t%s, %s;\n", dest, dest);
                                        /* dest > 0 if buf1 != buf2 */
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
                                        /* dest < 0 if buf1 != buf2 */
                                        fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
                                        /* dest == 1 (true) or -1 (false) */
                                        break;
				case '<':
					fprintf(assemblyFile, "# binary <:\n");
					fprintf(assemblyFile, "SLT\t%s, %s, %s;\n", dest, buf1, buf2);
                                        /* dest == 1 if buf1 < buf2, otherwise dest == 0 */
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
                                        /* dest == -1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
                                        /* dest == 1 (true) or -1 (false) */
                                        break;
				case _LEQ:
					fprintf(assemblyFile, "# binary LEQ:\n");
                                        fprintf(assemblyFile, "SGE\t%s, %s, %s;\n", dest, buf2, buf1);
                                        /* dest == 1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
                                        /* dest == -1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
                                        /* dest == 1 (true) or -1 (false) */
					break;
				case '>':
					fprintf(assemblyFile, "# binary >:\n");
					fprintf(assemblyFile, "SLT\t%s, %s, %s;\n", dest, buf2, buf1);
                                        /* dest == 1 if buf1 > buf2, otherwise dest == 0 */
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
                                        /* dest == -1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
                                        /* dest == 1 (true) or -1 (false) */
					break;
				case _GEQ:
					fprintf(assemblyFile, "# binary GEQ:\n");
                                        fprintf(assemblyFile, "SGE\t%s, %s, %s;\n", dest, buf1, buf2);
                                        /* dest == 1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, zero_reg, dest);
                                        /* dest == -1 (true) or 0 (false) */
                                        fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", dest, dest, true_reg, false_reg);
                                        /* dest == 1 (true) or -1 (false) */
					break;
				case '+':
					fprintf(assemblyFile, "# binary +:\n");
                                        fprintf(assemblyFile, "ADD\t%s, %s, %s;\n", dest, buf1, buf2);
					break;
				case '-':
					fprintf(assemblyFile, "# binary -:\n");
                                        fprintf(assemblyFile, "SUB\t%s, %s, %s;\n", dest, buf1, buf2);
					break;
				case '*':
					fprintf(assemblyFile, "# binary *:\n");
                                        fprintf(assemblyFile, "MUL\t%s, %s, %s;\n", dest, buf1, buf2);
					break;
				case '/':
					fprintf(assemblyFile, "# binary /:\n");
                                        fprintf(assemblyFile, "RCP\t%s, %s;\n", dest, buf2);
					fprintf(assemblyFile, "MUL\t%s, %s, %s;\n", dest, buf1, dest);
					break;
				case '^':
					fprintf(assemblyFile, "# binary ^:\n");
                                        fprintf(assemblyFile, "POW\t%s, %s, %s;\n", dest, buf1, buf2);
					break;
				default:
					strncpy(dest, "genCode_expr: Error: Unimplemented.", MAX_BUF_LEN);
                                        break;
			}

			free_tempreg(buf1);
      	                free_tempreg(buf2);

                    	strncpy(result, dest, MAX_BUF_LEN);

			break;
		case BOOL_NODE:
			get_tempreg(dest);
			if(ast->bool_lit.value == TRUE){
				fprintf(assemblyFile, "MOV\t%s, %s;\n", dest, true_reg);
			}
			else{
				fprintf(assemblyFile, "MOV\t%s, %s;\n", dest, false_reg);
			}
			strncpy(result, dest, MAX_BUF_LEN);
			break;
		case INT_NODE:
			get_tempreg(dest);
			sprintf(value, "%d", ast->int_lit.value);
			fprintf(assemblyFile, "MOV\t%s, %s;\n", dest, value);
			strncpy(result, dest, MAX_BUF_LEN);
			break;
		case FLOAT_NODE:
			get_tempreg(dest);
                        sprintf(value, "%f", ast->float_lit.value);
                        fprintf(assemblyFile, "MOV\t%s, %s;\n", dest, value);
                        strncpy(result, dest, MAX_BUF_LEN);
                        break;
		case VAR_NODE:
			var_to_assembly(result, ast->var.name, ast->var.ofs);
			break;
		case FUNCTION_NODE:
			fprintf(assemblyFile, "# function call:\n");
			arg_count = 0;
			genCode_args(ast->function.args_opt, &arg_count, buf1, buf2, buf3, buf4);
			get_tempreg(dest);
			
			if(ast->function.func == DP3){
				fprintf(assemblyFile, "DP3\t%s, %s, %s;\n", dest, buf1, buf2);
				free_tempreg(buf1);
                        	free_tempreg(buf2);
			}
			else if(ast->function.func == LIT){
				fprintf(assemblyFile, "LIT\t%s, %s;\n", dest, buf1);
				free_tempreg(buf1);
			}
			else{ /* RSQ */
				fprintf(assemblyFile, "RSQ\t%s, %s;\n", dest, buf1);
				free_tempreg(buf1);
			}

			strncpy(result, dest, MAX_BUF_LEN);
			break;
		case CONSTRUCTOR_NODE:
			fprintf(assemblyFile, "# constructor call:\n");
			arg_count = 0;
			genCode_args(ast->constructor.args_opt, &arg_count, buf1, buf2, buf3, buf4);
                        get_tempreg(dest);

			fprintf(assemblyFile, "MOV\t%s%s, %s;\n", dest, ".x", buf1);
			free_tempreg(buf1);
			if(arg_count > 1){ 
				fprintf(assemblyFile, "MOV\t%s%s, %s;\n", dest, ".y", buf2);
				free_tempreg(buf2);
			}
			if(arg_count > 2){
				fprintf(assemblyFile, "MOV\t%s%s, %s;\n", dest, ".z", buf3);
				free_tempreg(buf3);
			}
			if(arg_count > 3){
				fprintf(assemblyFile, "MOV\t%s%s, %s;\n", dest, ".w", buf4);
				free_tempreg(buf4);
			}

			strncpy(result, dest, MAX_BUF_LEN);
			break;
		default:
			strncpy(result, "genCode_expr: Error: Unimplemented.", MAX_BUF_LEN);
			break;
	}

	return;
}

/* Forward declarations for genCode_stmt */
static void genCode_dclns(node *ast);
static void genCode_stmts(node *ast, bool cond, char *condvar);

static void genCode_stmt(node *ast, bool cond, char *condvar){
	char buf1[MAX_BUF_LEN], buf2[MAX_BUF_LEN];
	char new_condvar1[MAX_BUF_LEN], new_condvar2[MAX_BUF_LEN];	

	if(ast == NULL) return;

	switch(ast->kind){
		case ASSIGNMENT_NODE:
			genCode_expr(ast->assign_stmt.var, buf1);
			genCode_expr(ast->assign_stmt.new_val, buf2);
			
			if(cond){ 
				fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", buf1, condvar, buf1, buf2);
			}
			else{
				fprintf(assemblyFile, "MOV\t%s, %s;\n", buf1, buf2);
			}
			
			free_tempreg(buf2);
			
			break;
		case IF_STATEMENT_NODE:
			/*
			 * Inputs for if/else logic:
 			 *      cond: We're in a conditional (if/else) statement.
			 *      condvar: Register to use in our CMP instruction.
			 *
			 *      if(EXPR1){ LEVEL1
			 *              if(EXPR2){ LEVEL2
			 *
			 *              }
			 *              else{ LEVEL2
			 *              
			 *              }
			 *      }
			 *      else{ LEVEL2
			 *
			 *      }
			 *
			 * 1. If not already conditional (ie. we are LEVEL1):
			 *	-Get 2 temporary registers (new_condvar1 and new_condvar2) 
			 *	-Store EXPR1 in new_condvar1, !EXPR1 in new_condvar2
			 *	-pass new_condvar1 to first statement
			 *	-pass new_condvar2 to second statement
			 * 2. If already conditional (ie. we are LEVELN, N > 1):
			 *	-Get 2 temporary registers (new_condvar1, new_condvar2)
			 *	-if condvar > 0 (ie. we are in a true branch) procede as in 1.
			 *	-else (we are in a false branch) new_condvar1 = -1 and new_condvar2 = -1.
			 */
			genCode_expr(ast->if_stmt.expr, buf1);
		
			fprintf(assemblyFile, "# if/else statement:\n");
	
			get_tempreg(new_condvar1);
			fprintf(assemblyFile, "MOV\t%s, %s;\n", new_condvar1, buf1);			
			free_tempreg(buf1);

			get_tempreg(new_condvar2);
			fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", new_condvar2, new_condvar1, true_reg, false_reg);

			if(cond){
				fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", new_condvar1, condvar, false_reg, new_condvar1);
				fprintf(assemblyFile, "CMP\t%s, %s, %s, %s;\n", new_condvar2, condvar, false_reg, new_condvar2);
			}

			genCode_stmt(ast->if_stmt.stmt, TRUE, new_condvar1);
			if(ast->if_stmt.opt_stmt != NULL)
				genCode_stmt(ast->if_stmt.opt_stmt, TRUE, new_condvar2);
		
			free_tempreg(new_condvar1);
			free_tempreg(new_condvar2);
			
			break;
		case SCOPE_NODE:
			genCode_dclns(ast->scope.declarations);
			genCode_stmts(ast->scope.statements, cond, condvar);
			break;
		default:
			break;
	}

	return;
}

static void genCode_stmts(node *ast, bool cond, char *condvar){
	
	if(ast == NULL) return;

	genCode_stmts(ast->statements.statements, cond, condvar);
	genCode_stmt(ast->statements.statement, cond, condvar);
	
	return;
}

static void genCode_dcln(node *ast){
	char buf[MAX_BUF_LEN];	
	struct st_entry *ste;

	ste = st_lookup(ast->st, ast->declaration.var_name, LOCAL);

	if(ste->is_cnst){
		/* init_val is either a literal or a uniform variable */
		if(ast->declaration.init_val->kind == VAR_NODE){
			genCode_expr(ast->declaration.init_val, buf);
			fprintf(assemblyFile, "PARAM\t%s = %s;\n", ast->declaration.var_name, buf);
			/* No need to free_tempreg(buf), since buf won't be a tempreg */
		}
		else{ /* it's a literal */
			if(ast->declaration.init_val->kind == BOOL_NODE){
				if(ast->declaration.init_val->bool_lit.value == TRUE)
					sprintf(buf, "%f", 1.0);
				else sprintf(buf, "%f", -1.0);
			}
			else if(ast->declaration.init_val->kind == INT_NODE){
				sprintf(buf, "%d", ast->declaration.init_val->int_lit.value);
			}
			else /* FLOAT_NODE */ 
				sprintf(buf, "%f", ast->declaration.init_val->float_lit.value);

			fprintf(assemblyFile, "PARAM\t%s = %s;\n", ast->declaration.var_name, buf);
		}
	}
	else{ /* not const */
		fprintf(assemblyFile, "TEMP\t%s;\n", ast->declaration.var_name);
		if(ast->declaration.init_val != NULL){
			genCode_expr(ast->declaration.init_val, buf);
                	fprintf(assemblyFile, "MOV\t%s, %s;\n", ast->declaration.var_name, buf);
                	free_tempreg(buf);
		}
	}

	return;
}

static void genCode_dclns(node *ast){
	
	if(ast == NULL) return;

	genCode_dclns(ast->declarations.declarations);
	genCode_dcln(ast->declarations.declaration);

	return;
}

/* No need for any assertions, we've already checked all that in our semantic analysis */
void genCode(node *ast){

	init_tempregs();
	fprintf(assemblyFile, "!!ARBfp1.0\n");
	init_utilregs();	
	genCode_stmt(ast, FALSE, NULL);
	fprintf(assemblyFile, "END");

	return;
}
