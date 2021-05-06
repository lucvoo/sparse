// SPDX-License-Identifier: MIT

#include "symbol.h"
#include "expression.h"
#include "linearize.h"
#include "flow.h"
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "gencode.h"


static void output_comment(void *state, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void output_comment(void *state, const char *fmt, ...)
{
	va_list args;

	if (!verbose)
		return;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

static int output_data(struct symbol *sym)
{
	const char* name = show_ident(sym->ident);

	// FIXME: section
	// FIXME: align
	printf("%s:\n", name);
	printf("\ttype = %d\n", sym->ctype.base_type->type);
	printf("\tmodif= %lx\n", sym->ctype.modifiers);
	// FIXME: size
	printf("\t.type\t%s, @object\n", name);
	printf("\n");

	return 0;
}

static void output_fn_static_sym(struct symbol_list* syms)
{
	struct symbol* sym;

	FOR_EACH_PTR(syms, sym) {
		if (sym->ctype.modifiers & MOD_STATIC)
			output_data(sym);
	}
	END_FOR_EACH_PTR(sym);
}


static void output_fn(struct entrypoint *ep)
{
	struct basic_block *bb;
	unsigned long generation = ++bb_generation;
	struct symbol *sym = ep->name;
	const char *name;

	output_fn_static_sym(ep->syms);

	name = show_ident(sym->ident);
	// FIXME: section
	// FIXME: align
	if (!(sym->ctype.modifiers & MOD_STATIC))
		printf("\t.global\t%s\n", name);
	printf("\t.type\t%s, %%function\n", name);
	printf("%s:\n", name);

	unssa(ep);

	bb = ep->entry->bb;
	output_comment(NULL, "\t# args: %d\n", pseudo_list_size(bb->needs));

	FOR_EACH_PTR(ep->bbs, bb) {
		if (bb->generation == generation)
			continue;
		codegen_bb(bb);
	}
	END_FOR_EACH_PTR(bb);

	printf("\n");
}


static
int sccompile(struct symbol_list *list)
{
	struct symbol *sym;
	FOR_EACH_PTR(list, sym) {
		struct entrypoint *ep;
		expand_symbol(sym);
		ep = linearize_symbol(sym);
		if (ep)
			output_fn(ep);
		else
			output_data(sym);
	}
	END_FOR_EACH_PTR(sym);

	return 0;
}


int main(int argc, char **argv)
{
	struct string_list *filelist = NULL;
	char *file;

	add_pre_buffer("#define __SPARSE_CC__\n");

	sccompile(sparse_initialize(argc, argv, &filelist));
	FOR_EACH_PTR_NOTAG(filelist, file) {
		sccompile(sparse(file));
	} END_FOR_EACH_PTR_NOTAG(file);
	return 0;
}
