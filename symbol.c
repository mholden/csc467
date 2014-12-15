#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "symbol.h"

symbol_table_t *st_new(){
	symbol_table_t *st;

        st = (symbol_table_t *) malloc(sizeof(struct symbol_table));
        if(st == NULL) return NULL;

	st->parent = st_curr;
        memset(st->entries, 0, MAX_ST_ENTRIES * sizeof(struct st_entry));
        st->num_entries = 0;

	return st;
}

void st_insert(char *var_name, type_t type, int is_cnst){
	/* Make sure var_name doesn't already exist */
	if(st_lookup(st_curr, var_name, LOCAL)){
		fprintf(errorFile, "SEMANTIC ERROR: Variable %s declared more than once in the current scope.\n", var_name);
		errorOccurred = TRUE;
	}

	st_curr->entries[st_curr->num_entries].var_name = strdup(var_name);
	st_curr->entries[st_curr->num_entries].type = type;
	st_curr->entries[st_curr->num_entries].is_cnst = is_cnst;
	st_curr->num_entries++;
	if(st_curr->num_entries >= MAX_ST_ENTRIES){
		/* TODO: Deal with this properly? */
		printf("st_insert: Warning: symbol table full\n");
	}
}

struct st_entry *st_lookup(symbol_table_t *st, char *var_name, scope_t scope){
	int i;

	while(st != NULL){
		for(i = 0; i < st->num_entries; i++){
			if(!strcmp(st->entries[i].var_name, var_name))
				return &st->entries[i];
		}	
		/* For local scope, don't search any of the symbol table's parents */
		if(scope == LOCAL) break;
		st = st->parent;
	}

	return NULL;
}
