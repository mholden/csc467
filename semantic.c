#include "semantic.h"
#include "ast.h"
#include "symbol.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// forward declare for sem_check_args
static void sem_check_expr(node *, type_t *);

static void sem_check_args(node *ast, int *arg_count, type_t *type1, type_t *type2, type_t *type3, type_t *type4){
	
	assert(ast != NULL);
        assert(ast->kind & ARGUMENTS_NODE);

	/* No function or constructor should have more than 4 arguments */
	if(*arg_count > 4) return;

	if(ast->arguments.args != NULL)
		sem_check_args(ast->arguments.args, arg_count, type1, type2, type3, type4);
	switch(*arg_count){
	  case 0:
		sem_check_expr(ast->arguments.expr, type1);
		break;
	  case 1:
		sem_check_expr(ast->arguments.expr, type2);
                break;
	  case 2:
		sem_check_expr(ast->arguments.expr, type3);
                break;
	  case 3:
		sem_check_expr(ast->arguments.expr, type4);
                break;
	  /* No need to support over 4 arguments */
	  default:
		break;
	}

	(*arg_count)++;

	return;
}

static void sem_check_expr(node *ast, type_t *type){
	struct st_entry *ste;
	type_t type1, type2, type3, type4;
	int arg_count;

	assert(ast != NULL);
        assert(ast->kind & EXPRESSION_NODE);

        switch(ast->kind){
	  case UNARY_EXPRESSION_NODE:
		sem_check_expr(ast->unary_expr.expr, &type1);

		/* Type check */	
		switch(ast->unary_expr.op){	
		  case '!': /* Logical unary operator */
			if(!(type1 & BOOL)){
				fprintf(errorFile, "SEMANTIC ERROR: Attempting to use unary logical operator '!' on type %s. Type must be bool.\n", type_strings[print_type_index(type1)]);
				errorOccurred = TRUE;
				ast->unary_expr.type = ANY;
				*type = ANY;
			}
			else{ /* We're good. */	
				ast->unary_expr.type = BOOL;
				*type = BOOL;
			}
			break;
		  case '-': /* Arithmetic unary operator */
			if((type1 & BOOL) && (type1 != ANY)){
				fprintf(errorFile, "SEMANTIC ERROR: Attempting to use unary arithmetic operator '-' on type %s. Type must be int, float, ivec or vec.\n", type_strings[print_type_index(type1)]);
                                errorOccurred = TRUE;
                                ast->unary_expr.type = ANY;
                                *type = ANY;
			}
			else{ /* We're good. */
				ast->unary_expr.type = type1;
                                *type = type1;
			}
			break;
		  default:
			printf("sem_check_expr: Unsupported unary op type.\n");
			break;
		}
		break;
	  case BINARY_EXPRESSION_NODE:
		sem_check_expr(ast->binary_expr.left, &type1);
		sem_check_expr(ast->binary_expr.right, &type2);

		/* Type check */
		switch(ast->binary_expr.op){
			case _AND:
			case _OR:/* Logical binary operators */
				/* 1. Must be bools */
				if(!((type1 & BOOL) && (type2 & BOOL))){
					fprintf(errorFile, "SEMANTIC ERROR: Both operands of logical binary expression need to be of bool type, and are not. "
							    "One is type %s, and the other is type %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
					errorOccurred = TRUE;
                       	 		ast->binary_expr.type = ANY;
					*type = ANY;
					return;
				}
				/* 2. Can't mix scalars and vectors */
				if((type1 == ANY) || (type2 == ANY)){
					/* If either are type ANY, then we're good. */
					ast->binary_expr.type = BOOL;
                                	*type = BOOL;
					return;
				}
				if(type1 == BOOL){
					if(type2 != BOOL){ /* type1 is scalar, type2 is vector */
						fprintf(errorFile, "SEMANTIC ERROR: Both operands of logical binary expression need to be the same (either scalar or vector), and are not. "
								    "One is type %s, and the other is type %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                                        	errorOccurred = TRUE;
                                        	ast->binary_expr.type = ANY;
                                        	*type = ANY;
                                        	return;
					}
				}
				else if(type2 == BOOL){ /* type1 is vector, type2 is scalar */
					fprintf(errorFile, "SEMANTIC ERROR: Both operands of logical binary expression need to be the same (either scalar or vector), and are not. "
                                                            "One is type %s, and the other is type %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                                      	errorOccurred = TRUE;
                                       	ast->binary_expr.type = ANY;
                                       	*type = ANY;
                                       	return;
				}

				/* We're good. */
				ast->binary_expr.type = BOOL;
				*type = BOOL;
				break;
			  case '+':
			  case '-': /* Arithmetic binary operators that accept scalars or vectors, but not mixes of the two */
				/* 1. Can't be bools */
				if(((type1 & BOOL) && (type1 != ANY)) || ((type2 & BOOL) && (type2 != ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Types can't be bool.\n");
					errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
				}
				/* 2. Must have same base types */
				if((type1 == ANY) && (type2 == ANY)){ /* Both are any */
					ast->binary_expr.type = ANY;
                                	*type = ANY;
					return;
				}
				else if(type1 == ANY){ /* type1 is any, type2 is not */
					ast->binary_expr.type = (type2 & INT) ? INT : FLOAT;
                                	*type = (type2 & INT) ? INT : FLOAT;
					return;
				}
				else if(type2 == ANY){ /* type2 is any, type1 is not */
					ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                	*type = (type1 & INT) ? INT : FLOAT;
					return;
				}
				/* Neither are ANY, so do the base type checking */
				if(type1 & INT){
					if(!(type2 & INT)){ /* type1 int type2 float */
						fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Must have same base types.\n");
                                        	errorOccurred = TRUE;
                                        	ast->binary_expr.type = ANY;
                                        	*type = ANY;
                                        	return;
					}
					/* 3. If 1 is scalar, 2 needs to be scalar */
					if(type1 == INT){
						if(type2 != INT){
							fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Can't mix scalars and vectors.\n");
                                                	errorOccurred = TRUE;
                                                	ast->binary_expr.type = ANY;
                                                	*type = ANY;
                                                	return;
						}
					}
					/* 4. If 1 is vector, 2 needs to be vector */
					else if(type2 == INT){
						fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Can't mix scalars and vectors.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
					}
				}
				else if(type2 & INT){ /* type1 float type2 int */
					fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Must have same base types.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
				}
				else{ /* type1 float type2 float */
					/* 3. If 1 is scalar, 2 needs to be scalar */
                                        if(type1 == FLOAT){
                                                if(type2 != FLOAT){
                                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Can't mix scalars and vectors.\n");
                                                        errorOccurred = TRUE;
                                                        ast->binary_expr.type = ANY;
                                                        *type = ANY;
                                                        return;
                                                }
                                        }
                                        /* 4. If 1 is vector, 2 needs to be vector */
                                        else if(type2 == FLOAT){
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '+' or '-'. Can't mix scalars and vectors.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
				}

				/* We're good. */
				ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                *type = (type1 & INT) ? INT : FLOAT;
				break;
			  case '*': /* Arithmetic binary operators that accept scalars, vectors, and mixes */
                                /* 1. Can't be bools */
                                if(((type1 & BOOL) && (type1 != ANY)) || ((type2 & BOOL) && (type2 != ANY))){
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '*'. Types can't be bool.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                /* 2. Must have same base types */
                                if((type1 == ANY) && (type2 == ANY)){ /* Both are any */
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                else if(type1 == ANY){ /* type1 is any, type2 is not */
                                        ast->binary_expr.type = (type2 & INT) ? INT : FLOAT;
                                        *type = (type2 & INT) ? INT : FLOAT;
                                        return;
                                }
                                else if(type2 == ANY){ /* type2 is any, type1 is not */
                                        ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                        *type = (type1 & INT) ? INT : FLOAT;
                                        return;
                                }
                                /* Neither are ANY, so do the base type checking */
                                if(type1 & INT){
                                        if(!(type2 & INT)){ /* type1 int type2 float */
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '*'. Must have same base types.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
                                }
                                else if(type2 & INT){ /* type1 float type2 int */
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '*'. Must have same base types.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }

                                /* We're good. */
                                ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                *type = (type1 & INT) ? INT : FLOAT;
                                break;
			  case '/':
			  case '^': /* Arithmetic binary operators that accept scalars only */
                                /* 1. Can't be bools */
                                if(((type1 & BOOL) && (type1 != ANY)) || ((type2 & BOOL) && (type2 != ANY))){
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '/' or '^'. Types can't be bool.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                /* 2. Must have same base types */
                                if((type1 == ANY) && (type2 == ANY)){ /* Both are any */
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                else if(type1 == ANY){ /* type1 is any, type2 is not */
                                        ast->binary_expr.type = (type2 & INT) ? INT : FLOAT;
                                        *type = (type2 & INT) ? INT : FLOAT;
                                        return;
                                }
                                else if(type2 == ANY){ /* type2 is any, type1 is not */
                                        ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                        *type = (type1 & INT) ? INT : FLOAT;
                                        return;
                                }
                                /* Neither are ANY, so do the base type checking */
                                if(type1 & INT){
                                        if(!(type2 & INT)){ /* type1 int type2 float */
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '/' or '^'. Must have same base types.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
					else if((type1 != INT) || (type2 != INT)){ /* Both are ints, but one (or both) is/are not scalar */
						fprintf(errorFile, "SEMANTIC ERROR: Binary op '/' or '^'. Must both be scalars.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
					}
                                }
                                else if(type2 & INT){ /* type1 float type2 int */
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '/' or '^'. Must have same base types.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
				else if((type1 != FLOAT) || (type2 != FLOAT)){ /* Both are floats, but one (or both) is/are not scalar */
                                     	fprintf(errorFile, "SEMANTIC ERROR: Binary op '/' or '^'. Must both be scalars.\n");
                               	        errorOccurred = TRUE;
                     	                ast->binary_expr.type = ANY;
               	                        *type = ANY;
       	                                return;
				}

                                /* We're good. */
                                ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                *type = (type1 & INT) ? INT : FLOAT;
				break;
			  case '<':
                          case _LEQ:
                          case '>':
                          case _GEQ: /* Comparison binary operators that accept scalars only */
                                /* 1. Can't be bools */
                                if(((type1 & BOOL) && (type1 != ANY)) || ((type2 & BOOL) && (type2 != ANY))){
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '<', '<=', '>', or '>='. Types can't be bool.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                /* 2. Must have same base types */
                                if((type1 == ANY) && (type2 == ANY)){ /* Both are any */
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                else if(type1 == ANY){ /* type1 is any, type2 is not */
                                        ast->binary_expr.type = (type2 & INT) ? INT : FLOAT;
                                        *type = (type2 & INT) ? INT : FLOAT;
                                        return;
                                }
                                else if(type2 == ANY){ /* type2 is any, type1 is not */
                                        ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                        *type = (type1 & INT) ? INT : FLOAT;
                                        return;
                                }
                                /* Neither are ANY, so do the base type checking */
                                if(type1 & INT){
                                        if(!(type2 & INT)){ /* type1 int type2 float */
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '<', '<=', '>', or '>='. Must have same base types.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
                                        else if((type1 != INT) || (type2 != INT)){ /* Both are ints, but one (or both) is/are not scalar */
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '<', '<=', '>', or '>='. Must both be scalars.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
                                }
                                else if(type2 & INT){ /* type1 float type2 int */
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '<', '<=', '>', or '>='. Must have same base types.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                else if((type1 != FLOAT) || (type2 != FLOAT)){ /* Both are floats, but one (or both) is/are not scalar */
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '<', '<=', '>', or '>='. Must both be scalars.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }

				/* We're good. */
                                ast->binary_expr.type = BOOL;
                                *type = BOOL;
				break;
			  case _EQ:
			  case _NEQ: /* Comparison binary operators that accept scalars or vectors, but not mixes of the two */
                                /* 1. Can't be bools */
                                if(((type1 & BOOL) && (type1 != ANY)) || ((type2 & BOOL) && (type2 != ANY))){
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Types can't be bool.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                /* 2. Must have same base types */
                                if((type1 == ANY) && (type2 == ANY)){ /* Both are any */
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                else if(type1 == ANY){ /* type1 is any, type2 is not */
                                        ast->binary_expr.type = (type2 & INT) ? INT : FLOAT;
                                        *type = (type2 & INT) ? INT : FLOAT;
                                        return;
                                }
                                else if(type2 == ANY){ /* type2 is any, type1 is not */
                                        ast->binary_expr.type = (type1 & INT) ? INT : FLOAT;
                                        *type = (type1 & INT) ? INT : FLOAT;
                                        return;
                                }
                                /* Neither are ANY, so do the base type checking */
                                if(type1 & INT){
                                        if(!(type2 & INT)){ /* type1 int type2 float */
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Must have same base types.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
                                        /* 3. If 1 is scalar, 2 needs to be scalar */
                                        if(type1 == INT){
                                                if(type2 != INT){
                                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Can't mix scalars and vectors.\n");
                                                        errorOccurred = TRUE;
                                                        ast->binary_expr.type = ANY;
                                                        *type = ANY;
                                                        return;
                                                }
                                        }
                                        /* 4. If 1 is vector, 2 needs to be vector */
                                        else if(type2 == INT){
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Can't mix scalars and vectors.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
                                }
				else if(type2 & INT){ /* type1 float type2 int */
                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Must have same base types.\n");
                                        errorOccurred = TRUE;
                                        ast->binary_expr.type = ANY;
                                        *type = ANY;
                                        return;
                                }
                                else{ /* type1 float type2 float */
                                        /* 3. If 1 is scalar, 2 needs to be scalar */
                                        if(type1 == FLOAT){
                                                if(type2 != FLOAT){
                                                        fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Can't mix scalars and vectors.\n");
                                                        errorOccurred = TRUE;
                                                        ast->binary_expr.type = ANY;
                                                        *type = ANY;
                                                        return;
                                                }
                                        }
                                        /* 4. If 1 is vector, 2 needs to be vector */
                                        else if(type2 == FLOAT){
                                                fprintf(errorFile, "SEMANTIC ERROR: Binary op '==' or '!='. Can't mix scalars and vectors.\n");
                                                errorOccurred = TRUE;
                                                ast->binary_expr.type = ANY;
                                                *type = ANY;
                                                return;
                                        }
                                }

                                /* We're good. */
                                ast->binary_expr.type = BOOL;
                                *type = BOOL;
				break;
			  default:
				printf("sem_check_expr: Unsupported op.\n");
				break;
		}
		break;
          case BOOL_NODE:
                ast->bool_lit.type = BOOL;
                *type = BOOL;
                break;
	  case INT_NODE:
		ast->int_lit.type = INT;
		*type = INT;
                break;
          case FLOAT_NODE:
		ast->float_lit.type = FLOAT;
                *type = FLOAT;
		break;
          case VAR_NODE:
		ste = st_lookup(ast->st, ast->var.name, GLOBAL);
		if(ste == NULL){
			fprintf(errorFile, "SEMANTIC ERROR: Undeclared variable %s.\n", ast->var.name);
			errorOccurred = TRUE;
			ast->var.type = ANY;
			*type = ANY;
		} else{
			ast->var.type = ste->type;
			*type = ste->type;
		
			/*variable indexing check*/
			if(*type == VEC2 || *type == VEC3 || *type == VEC4){ //on a type by type basis
				if(ast->var.ofs != -1){				
					switch(*type){
						case VEC2: //specific cases for each offset for vector
							if(ast->var.ofs != 0 && ast->var.ofs != 1)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						case VEC3:
							if(ast->var.ofs != 0 && ast->var.ofs != 1 && ast->var.ofs != 2)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						case VEC4:
							if(ast->var.ofs != 0 && ast->var.ofs != 1 && ast->var.ofs != 2 && ast->var.ofs != 3)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						default:
							break;
					}
					*type = FLOAT;
				}
			}
			else if(*type == IVEC2 || *type == IVEC3 || *type == IVEC4){
				if(ast->var.ofs != -1){					
					switch(*type){
						case IVEC2:
							if(ast->var.ofs != 0 && ast->var.ofs != 1)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						case IVEC3:
							if(ast->var.ofs != 0 && ast->var.ofs != 1 && ast->var.ofs != 2)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						case IVEC4:
							if(ast->var.ofs != 0 && ast->var.ofs != 1 && ast->var.ofs != 2 && ast->var.ofs != 3)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						default:
							break;
					}
					*type = INT;
				}
			}
			else if(*type == BVEC2 || *type == BVEC3 || *type == BVEC4){
				if(ast->var.ofs != -1){
					switch(*type){
						case BVEC2:
							if(ast->var.ofs != 0 && ast->var.ofs != 1)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						case BVEC3:
							if(ast->var.ofs != 0 && ast->var.ofs != 1 && ast->var.ofs != 2)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						case BVEC4:
							if(ast->var.ofs != 0 && ast->var.ofs != 1 && ast->var.ofs != 2 && ast->var.ofs != 3)
								fprintf(errorFile, "SEMANTIC ERROR: Invalid vector index.\n");
							break;
						default:
							break;
					}
					*type = BOOL;
				}
			}
		}
		break;
	  case FUNCTION_NODE:
		if(ast->function.args_opt == NULL){
			fprintf(errorFile, "SEMANTIC ERROR: Function %s has zero arguments.\n", func_strings[print_func_index(ast->function.func)]);
                        errorOccurred = TRUE;
			*type = ANY;
			return;
                } else {
			arg_count = 0;
			sem_check_args(ast->function.args_opt, &arg_count, &type1, &type2, &type3, &type4);
		}
		switch(ast->function.func){
		  case DP3: /* 2 arguments, either vec3/4s or ivec3/4s; return type is dependant on argument types */
			if(arg_count != 2){
				fprintf(errorFile, "SEMANTIC ERROR: DP3 function needs 2 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = ANY;
				return;
			}
			else{ /* Right number of args; type check */
				if((type1 == ANY) && (type2 == ANY)){
                                        *type = ANY;
					return;
                                }
                                else if(type1 == ANY){ /* type1 is any, type2 is not */
					if((type2 & INT) && (type2 == IVEC3 || type2 == IVEC4)){
						/* Good. */
						*type = INT;
						return;	
					}
					else if((type2 & FLOAT) && (type2 == VEC3 || type2 == VEC4)){
						/* Good. */
						*type = FLOAT;
						return;
					}
					else{ /* Not good. */
						fprintf(errorFile, "SEMANTIC ERROR: DP3 function need arguments to be ivec3s, ivec4s, vec3s, or vec4s. Argument types are: %s, %s\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        			errorOccurred = TRUE;
						*type = ANY;
						return;
					}
				} else if(type2 == ANY){ /* type2 is any, type1 is not */
					if((type1 & INT) && (type1 == IVEC3 || type1 == IVEC4)){
						/* Good. */
						*type = INT;
						return;	
					}
					else if((type1 & FLOAT) && (type1 == VEC3 || type1 == VEC4)){
						/* Good. */
						*type = FLOAT;
						return;
					}
					else{ /* Not good. */
						fprintf(errorFile, "SEMANTIC ERROR: DP3 function need arguments to be ivec3s, ivec4s, vec3s, or vec4s. Argument types are: %s, %s\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        			errorOccurred = TRUE;
						*type = ANY;
						return;
					}
				}
				else{ /* Neither are type ANY */
					if(type1 != type2){
						fprintf(errorFile, "SEMANTIC ERROR: DP3 function need arguments to be ivec3s, ivec4s, vec3s, or vec4s. Argument types are: %s, %s\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        			errorOccurred = TRUE;
						*type = ANY;
						return;
					}
					else{
						if((type1 & INT) && (type1 == IVEC3 || type1 == IVEC4)){
							/* Good. */
							*type = INT;
							return;	
						}
						else if((type1 & FLOAT) && (type1 == VEC3 || type1 == VEC4)){
							/* Good. */
							*type = FLOAT;
							return;
						}
						else{ /* Not good. */
							fprintf(errorFile, "SEMANTIC ERROR: DP3 function need arguments to be ivec3s, ivec4s, vec3s, or vec4s. Argument types are: %s, %s\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        				errorOccurred = TRUE;
							*type = ANY;
							return;
						}
					} 
				} 
			}
			break;
		  case LIT:
			if(arg_count != 1){
				fprintf(errorFile, "SEMANTIC ERROR: LIT function needs 1 argument, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = VEC4;
				return;
			}
			else{
				if(!((type1 == VEC4) || (type1 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: LIT function needs argument type vec4, and has argument type %s.\n", type_strings[print_type_index(type1)]);
                        		errorOccurred = TRUE;
					*type = VEC4;
					return;
				} else{ /* Good. */
					*type = VEC4;
					return;
				}
			}
			break;
		  case RSQ:
			if(arg_count != 1){
				fprintf(errorFile, "SEMANTIC ERROR: RSQ function needs 1 argument, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = FLOAT;
				return;
			}
			else{
				if(!((type1 == INT) || (type1 == FLOAT) || (type1 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: RSQ function needs argument type int or float, and has argument type %s.\n", type_strings[print_type_index(type1)]);
                        		errorOccurred = TRUE;
					*type = FLOAT;
					return;
				} else{ /* Good. */
					*type = FLOAT;
					return;
				}
			}
			break;
		  default:
			fprintf(outputFile, "sem_check_expr: Warning: Unexpected function type.\n");
			break;
		}
		break;
	  case CONSTRUCTOR_NODE:
		if(ast->constructor.args_opt == NULL){
			fprintf(errorFile, "SEMANTIC ERROR: Constructor for %s has zero arguments.\n", type_strings[print_type_index(ast->constructor.type)]);
                        errorOccurred = TRUE;
			*type = ANY;
			return;
                } else {
			arg_count = 0;
			sem_check_args(ast->constructor.args_opt, &arg_count, &type1, &type2, &type3, &type4);
		}
		switch(ast->constructor.type){
		  case INT:
			if(arg_count != 1){
				fprintf(errorFile, "SEMANTIC ERROR: INT constructor needs 1 argument, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = INT;
				return;
			} else{
				if(!((type1 == INT) || (type1 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: INT constructor needs argument type int, and has argument type %s.\n", type_strings[print_type_index(type1)]);
                        		errorOccurred = TRUE;
					*type = INT;
					return;
				}
				else{ /* Good. */
					*type = INT;
					return;
				}
			}
			break;
		  case IVEC2:
			if(arg_count != 2){
				fprintf(errorFile, "SEMANTIC ERROR: IVEC2 constructor needs 2 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = IVEC2;
				return;
			} else{
				if(!(((type1 == INT) || (type1 == ANY)) && ((type2 == INT) || type2 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: IVEC2 constructor needs arguments type int, and has arguments type %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        		errorOccurred = TRUE;
					*type = IVEC2;
					return;
				} else{ /* Good. */
					*type = IVEC2;
					return;
				}
			}
			break;
		  case IVEC3:
			if(arg_count != 3){
				fprintf(errorFile, "SEMANTIC ERROR: IVEC3 constructor needs 3 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = IVEC3;
				return;
			} else{
				if(!( ( (type1 == INT) || (type1 == ANY) ) && ( (type2 == INT) || (type2 == ANY) )
				   && ( (type3 == INT) || (type3 == ANY) ) )) {
					fprintf(errorFile, "SEMANTIC ERROR: IVEC3 constructor needs arguments type int, and has arguments type %s, %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)], type_strings[print_type_index(type3)]);
                        		errorOccurred = TRUE;
					*type = IVEC3;
					return;
				} else{ /* Good. */
					*type = IVEC3;
					return;
				}
			}
			break;
		  case IVEC4:
			if(arg_count != 4){
				fprintf(errorFile, "SEMANTIC ERROR: IVEC4 constructor needs 4 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = IVEC4;
				return;
			} else{
				if(!( ( (type1 == INT) || (type1 == ANY) ) && ( (type2 == INT) || (type2 == ANY) )
				   && ( (type3 == INT) || (type3 == ANY) ) && ( (type4 == INT) || (type4 == ANY) ) )) {
					fprintf(errorFile, "SEMANTIC ERROR: IVEC4 constructor needs arguments type int, and has arguments type %s, %s, %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)], type_strings[print_type_index(type3)], type_strings[print_type_index(type4)]);
                        		errorOccurred = TRUE;
					*type = IVEC4;
					return;
				} else{ /* Good. */
					*type = IVEC4;
					return;
				}
			}
			break;	
		  case FLOAT:
			if(arg_count != 1){
				fprintf(errorFile, "SEMANTIC ERROR: FLOAT constructor needs 1 argument, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = FLOAT;
				return;
			} else{
				if(!((type1 == FLOAT) || (type1 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: FLOAT constructor needs argument type float, and has argument type %s.\n", type_strings[print_type_index(type1)]);
                        		errorOccurred = TRUE;
					*type = FLOAT;
					return;
				}
				else{ /* Good. */
					*type = FLOAT;
					return;
				}
			}
			break;
		  case VEC2:
			if(arg_count != 2){
				fprintf(errorFile, "SEMANTIC ERROR: VEC2 constructor needs 2 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = VEC2;
				return;
			} else{
				if(!(((type1 == FLOAT) || (type1 == ANY)) && ((type2 == FLOAT) || type2 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: VEC2 constructor needs arguments type float, and has arguments type %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        		errorOccurred = TRUE;
					*type = VEC2;
					return;
				} else{ /* Good. */
					*type = VEC2;
					return;
				}
			}
			break;
		  case VEC3:
			if(arg_count != 3){
				fprintf(errorFile, "SEMANTIC ERROR: VEC3 constructor needs 3 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = VEC3;
				return;
			} else{
				if(!( ( (type1 == FLOAT) || (type1 == ANY) ) && ( (type2 == FLOAT) || (type2 == ANY) )
				   && ( (type3 == FLOAT) || (type3 == ANY) ) )) {
					fprintf(errorFile, "SEMANTIC ERROR: VEC3 constructor needs arguments type float, and has arguments type %s, %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)], type_strings[print_type_index(type3)]);
                        		errorOccurred = TRUE;
					*type = VEC3;
					return;
				} else{ /* Good. */
					*type = VEC3;
					return;
				}
			}
			break;
		  case VEC4:
			if(arg_count != 4){
				fprintf(errorFile, "SEMANTIC ERROR: VEC4 constructor needs 4 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = VEC4;
				return;
			} else{
				if(!( ( (type1 == FLOAT) || (type1 == ANY) ) && ( (type2 == FLOAT) || (type2 == ANY) )
				   && ( (type3 == FLOAT) || (type3 == ANY) ) && ( (type4 == FLOAT) || (type4 == ANY) ) )) {
					fprintf(errorFile, "SEMANTIC ERROR: VEC4 constructor needs arguments type float, and has arguments type %s, %s, %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)], type_strings[print_type_index(type3)], type_strings[print_type_index(type4)]);
                        		errorOccurred = TRUE;
					*type = VEC4;
					return;
				} else{ /* Good. */
					*type = VEC4;
					return;
				}
			}
			break;
		  case BOOL:
			if(arg_count != 1){
				fprintf(errorFile, "SEMANTIC ERROR: BOOL constructor needs 1 argument, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = BOOL;
				return;
			} else{
				if(!((type1 == BOOL) || (type1 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: BOOL constructor needs argument type bool, and has argument type %s.\n", type_strings[print_type_index(type1)]);
                        		errorOccurred = TRUE;
					*type = BOOL;
					return;
				}
				else{ /* Good. */
					*type = BOOL;
					return;
				}
			}
			break;
		  case BVEC2:
			if(arg_count != 2){
				fprintf(errorFile, "SEMANTIC ERROR: BVEC2 constructor needs 2 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = BVEC2;
				return;
			} else{
				if(!(((type1 == BOOL) || (type1 == ANY)) && ((type2 == BOOL) || type2 == ANY))){
					fprintf(errorFile, "SEMANTIC ERROR: BVEC2 constructor needs arguments type bool, and has arguments type %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
                        		errorOccurred = TRUE;
					*type = BVEC2;
					return;
				} else{ /* Good. */
					*type = BVEC2;
					return;
				}
			}
			break;
		  case BVEC3:
			if(arg_count != 3){
				fprintf(errorFile, "SEMANTIC ERROR: BVEC3 constructor needs 3 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = BVEC3;
				return;
			} else{
				if(!( ( (type1 == BOOL) || (type1 == ANY) ) && ( (type2 == BOOL) || (type2 == ANY) )
				   && ( (type3 == BOOL) || (type3 == ANY) ) )) {
					fprintf(errorFile, "SEMANTIC ERROR: BVEC3 constructor needs arguments type bool, and has arguments type %s, %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)], type_strings[print_type_index(type3)]);
                        		errorOccurred = TRUE;
					*type = BVEC3;
					return;
				} else{ /* Good. */
					*type = BVEC3;
					return;
				}
			}
			break;
		  case BVEC4:
			if(arg_count != 4){
				fprintf(errorFile, "SEMANTIC ERROR: BVEC4 constructor needs 4 arguments, and has %d arguments.\n", arg_count);
                        	errorOccurred = TRUE;
				*type = BVEC4;
				return;
			} else{
				if(!( ( (type1 == BOOL) || (type1 == ANY) ) && ( (type2 == BOOL) || (type2 == ANY) )
				   && ( (type3 == BOOL) || (type3 == ANY) ) && ( (type4 == BOOL) || (type4 == ANY) ) )) {
					fprintf(errorFile, "SEMANTIC ERROR: BVEC4 constructor needs arguments type bool, and has arguments type %s, %s, %s, %s.\n", type_strings[print_type_index(type1)], type_strings[print_type_index(type2)], type_strings[print_type_index(type3)], type_strings[print_type_index(type4)]);
                        		errorOccurred = TRUE;
					*type = BVEC4;
					return;
				} else{ /* Good. */
					*type = BVEC4;
					return;
				}
			}
			break;
		  default:
			fprintf(outputFile, "sem_check_expr: Warning: constructor type is ANY.\n");
			break;
		}
		break;
          default:
                printf("sem_check_expr: Unsupported expression type.\n");
                break;
	}
}

/* Forward declarations for sem_check_stmt */
static void sem_check_dclns(node *);
static void sem_check_stmts(node *);

static void sem_check_stmt(node *ast){
	type_t type1, type2;
	struct st_entry *ste;

	if(ast == NULL)	return;
	assert(ast->kind & STATEMENT_NODE);	

	switch(ast->kind){
	  case ASSIGNMENT_NODE:
		sem_check_expr(ast->assign_stmt.var, &type1);
		sem_check_expr(ast->assign_stmt.new_val, &type2);

		/* Can't reassign const variables */
		ste = st_lookup(ast->assign_stmt.var->st, ast->assign_stmt.var->var.name, GLOBAL);
		if(ste == NULL){
			printf("sem_check_stmt: Warning: st_lookup failed on variable %s.\n", ast->assign_stmt.var->var.name);
		}
		else{
			if(ste->is_cnst){
				fprintf(errorFile, "SEMANTIC ERROR: Can't reassign const variables. Trying to reassign const variable %s.\n", ast->assign_stmt.var->var.name);
                        	errorOccurred = TRUE;
			}
		}
		
		/* Type check */
		if(!(type1 & type2)){
			fprintf(errorFile, "SEMANTIC ERROR: Type mismatch - trying to assign variable %s of type %s with type %s.\n", 
					ast->assign_stmt.var->var.name, type_strings[print_type_index(type1)], type_strings[print_type_index(type2)]);
			errorOccurred = TRUE;
		}

		break;
	  case IF_STATEMENT_NODE:
		sem_check_expr(ast->if_stmt.expr, &type1);
		sem_check_stmt(ast->if_stmt.stmt);
		if(ast->if_stmt.opt_stmt != NULL)
			sem_check_stmt(ast->if_stmt.opt_stmt);

		/* Type check */
		if((type1 != BOOL) && (type1 != ANY)){
			fprintf(errorFile, "SEMANTIC ERROR: Conditional expression for if statement is of type %s, needs to be of type bool.\n",
					type_strings[print_type_index(type1)]);
			errorOccurred = TRUE;
		}
		
		break;
	  case SCOPE_NODE:
		sem_check_dclns(ast->scope.declarations);
        	sem_check_stmts(ast->scope.statements);
		/* Do whatever semantic checks need to be done for a scope node */
		break;
	  default:
		printf("sem_check_stmt: Unsupported statement kind.\n");
		break;
	}
}

static void sem_check_stmts(node *ast){

	if(ast == NULL) return;
        assert(ast->kind == STATEMENTS_NODE);

        sem_check_stmts(ast->statements.statements);
        sem_check_stmt(ast->statements.statement);

        /* Do whatever semantic checks need to be done for a statements node */
}

static void sem_check_dcln(node *ast){
	type_t type;
	struct st_entry *ste;

        assert(ast != NULL);
	assert(ast->kind == DECLARATION_NODE);

	if(ast->declaration.init_val != NULL){
        	sem_check_expr(ast->declaration.init_val, &type);
	
		/* 
		 * Const declarations must be initialized with a literal or uniform variable. 
		 * Note: Parser ensures that all const variables ARE initialized, so we just
		 * need to check that they are initialized with the correct variable/type.
		 */
                ste = st_lookup(ast->st, ast->declaration.var_name, LOCAL);
                if(ste == NULL){
                        printf("sem_check_dcln: Warning: st_lookup failed on variable %s.\n", ast->declaration.var_name);
                }
                else{
                        if(ste->is_cnst){
                                if(!((ast->declaration.init_val->kind == INT_NODE) || (ast->declaration.init_val->kind == FLOAT_NODE) || (ast->declaration.init_val->kind == BOOL_NODE))){ /* It's not a literal */
					/* Check if uniform? */
					if(ast->declaration.init_val->kind == VAR_NODE){
						if( !( !strcmp(ast->declaration.init_val->var.name, "gl_Light_Half")
						     || !strcmp(ast->declaration.init_val->var.name, "gl_Light_Ambient")
						     || !strcmp(ast->declaration.init_val->var.name, "gl_Material_Shininess")
						     || !strcmp(ast->declaration.init_val->var.name, "env1")
						     || !strcmp(ast->declaration.init_val->var.name, "env2")
						     || !strcmp(ast->declaration.init_val->var.name, "env3")
														 )){
							fprintf(errorFile, "SEMANTIC ERROR: Must assign const variables with literals or uniform variables. " 
							   "Trying to assign const variable %s with a non literal or non uniform variable.\n", ast->declaration.var_name);
                                			errorOccurred = TRUE;
						}
					}
					else{
						fprintf(errorFile, "SEMANTIC ERROR: Must assign const variables with literals or uniform variables. " 
							   "Trying to assign const variable %s with a non literal or non uniform variable.\n", ast->declaration.var_name);
                                		errorOccurred = TRUE;
					}
				}
                        }
                }

		/* Type check */
		if(!(ste->type & type)){
			fprintf(errorFile, "SEMANTIC ERROR: Type mismatch - trying to assign variable %s of type %s with type %s.\n",
                                        ast->declaration.var_name, type_strings[print_type_index(ste->type)], type_strings[print_type_index(type)]);
                        errorOccurred = TRUE;
		}
	}
}

static void sem_check_dclns(node *ast){
	
	if(ast == NULL) return;
	assert(ast->kind == DECLARATIONS_NODE);

	sem_check_dclns(ast->declarations.declarations);
	sem_check_dcln(ast->declarations.declaration);

	/* Do whatever semantic checks need to be done for a declarations node */
}

int semantic_check(node *ast) {
	
	assert(ast != NULL);
	assert(ast->kind == SCOPE_NODE);

	/* Expecting a scope, which is a statement */
	sem_check_stmt(ast);  

	return 0;
}
