// SPDX-License-Identifier: MIT

#ifndef	INSNCODE_H
#define	INSNCODE_H

#include "linearize.h"
#include "ptrlist.h"

enum insncode {
#define	INSN(A, N)	INSN_ ## A,
#define	OP(A, N)	INSN_ ## A = OP_ ## A,
#include "insncode.def"
#undef	INSN
#undef	OP
	INSN_NBR,
};

#endif
