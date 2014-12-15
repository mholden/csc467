#ifndef _SYMBOL_H
#define _SYMBOL_H

#include "common.h"

#define MAX_ST_ENTRIES 50 /* This is probably enough? */

struct st_entry{
	char *var_name;
	type_t type;
	int is_cnst;
};

struct symbol_table{
	struct symbol_table *parent;
	struct st_entry entries[MAX_ST_ENTRIES];
	int num_entries;
};

/* 
 * Attach a new symbol table to our "cactus" of symbol tables: 
 *	To be called at beginning of scope.
 */
symbol_table_t *st_new();

/* Insert a new entry into st_curr */
void st_insert(char *var_name, type_t type, int is_cnst);

/* Lookup an entry */
struct st_entry *st_lookup(symbol_table_t *st, char *var_name, scope_t scope);

#endif /* _SYMBOL_H */
