// SPDX-License-Identifier: MIT

#include <string.h>
#include "insncode.h"
#include "codegen.h"


static const struct term opcodes[] = {
#define	INSN(O, N)	[INSN_ ## O] = { #O, N },
#define	OP(O, N)	[INSN_ ## O] = { #O, N },
#include "insncode.def"
#undef INSN
#undef OP
};



int lookup_term(const char *name)
{
	int i;

	for (i = 0; i < (sizeof(opcodes) / sizeof(opcodes[0])); i++) {
		if (!opcodes[i].name)
			continue;
		if (strcmp(name, opcodes[i].name) == 0)
			return i;
	}

	return -1;
}

const char *opcode_name(unsigned int op)
{
	return opcodes[op].name;
}

int check_term(unsigned int idx, int arity)
{
	if (idx >= INSN_NBR)
		return 0;
	if (opcodes[idx].arity != arity)
		return 0;
	return 1;
}
