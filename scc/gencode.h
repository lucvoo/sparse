// SPDX-License-Identifier: MIT

#ifndef	GENCODE_H
#define	GENCODE_H

#include "insncode.h"

struct pseudo;
struct instruction;

#define	NBR_KIDS	3

struct cg_state {
	enum insncode		op;
	struct instruction	*insn;
	pseudo_t		src;
	struct cg_state		*kids[NBR_KIDS];
};

// gen-XXX.c
struct cg_state *alloc_state(int op, struct pseudo *src, struct instruction *insn);

void label_state(struct cg_state *s);
void reduce_state(struct cg_state *s, int nt);

// translate.c
void codegen_bb(struct basic_block *bb);

#endif
