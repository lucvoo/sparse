// SPDX-License-Identifier: MIT

#include "gencode.h"
#include "../expression.h" // only needed for emit SETVAL
#include <ctype.h>
#include <stdio.h>

#if 0
#define TRACE 1
#define	trace_reduce(fmt, ...)	printf("#%*sreducing %s with rule %d " fmt, tlevel, "", name_opcode(s->op), rule, ##__VA_ARGS__)
#define	trace_emit(tmpl)	if (tmpl && *tmpl != '%') printf("\n#emit: '%s'\n", tmpl)
#else
#define TRACE 0
#define	trace_reduce(fmt, ...)	do {} while (0)
#define	trace_emit(tmpl)	do {} while (0)
#endif

struct cg_state *alloc_state(int op, pseudo_t src, struct instruction *insn)
{
	struct state *s = malloc(sizeof(struct state));

	if (!s) die("out of memory");

	s->op = op;
	s->src = src;
	s->insn = insn;
	s->kids[0] = s->kids[1] = NULL;
	memset(s->rules, 0x00, sizeof(s->rules)); // FIXME: needed ?
	memset(s->costs, 0xff, sizeof(s->costs));

	if (insn && !insn->cg_state)
		insn->cg_state = &s->ext;

	return &s->ext;
}


#define	ARG_MAX	('0' + NBR_KIDS)

static const char *emit_pseudo(pseudo_t p, int force_reg)
{
#define	BSIZE	64
	static char buffer[4][BSIZE];
	static int n;

	if (!p)
		return "??";

	if (force_reg && p->type != PSEUDO_ARG) {
		char *buf = buffer[++n & 3];
		snprintf(buf, BSIZE, "%%r%d", p->nr);
		return buf;
	}

	return show_pseudo(p);
}

static void emit_tmpl(struct state *s, int rule)
{
	const struct rule_info *rinfo = &rules_info[rule];
	const char *tmpl = rinfo->action;
	struct state *k;
	int nt;
	int c;

	trace_emit(tmpl);

	if (!tmpl) {
		int rule, nt;

		if (!rinfo->chain)
			return;

		nt = rinfo->rhs[1];
	        rule = s->rules[nt];
		return emit_tmpl(s, rule);
	}

	while ((c = *tmpl++)) {
		const char *out;
		int force_reg;
		int type;
		int arg;

		switch (c) {
		case '%':
			break;

		case ';':
			// ignore following spaces
			while (isspace(*tmpl))
				tmpl++;

			// and emit a newline
			putchar('\n');
			putchar('\t');

			continue;

		case '\\':
			switch (*tmpl) {
			case 't':
				tmpl++;
				putchar('\t');
				break;
			case 'r':
				tmpl++;
				putchar('\r');
				break;
			default:
				break;
			}
		default:
			putchar(c);
			continue;
		}

		force_reg = 0;
		out = "??";
		// process the '%...'
		type = *tmpl++;
		switch (type) {
		case 'r':	// register
			force_reg = 1;
		case 'l':	// label
		case 'c':	// constant
			switch ((arg = *tmpl++)) {
			case 't':
				if (s->src) {
					out = emit_pseudo(s->src, 1);
				}
				break;
			case 'p':
				if (s->src) {
					s->src->type = PSEUDO_REG;
					out = show_pseudo(s->src);
				}
				break;
			case 'q':
				if (s->src) {
					out = show_pseudo(s->src);
					s->src->type = PSEUDO_REG;
				}
				break;
			case 'd':
				if (s->insn)
					out = show_pseudo(s->insn->target);
				break;
			case '0':
				if (s->src)
					out = show_pseudo(s->src);
				break;
			case '1' ... ARG_MAX: {
				struct state *p = s->kids[arg-'1'];

				arg = *tmpl;
				while (arg >= '1' && arg <= ARG_MAX) {
					p = p->kids[arg-'1'];
					arg = *++tmpl;
				}

				if (p && p->src)
					out = emit_pseudo(p->src, force_reg);
				break;

			case 'z':	// for ARM64
				out = "wzr";	// FIXME: or "xzr"
				break;
			}

			default:
				/* FIXME */;
			}

			fputs(out, stdout);
			break;

		case 'b':	// branch
			printf(".L%d", s->insn->bb_true->nr);
			if (s->insn->bb_false)
				printf(", .L%d", s->insn->bb_false->nr);

			break;

		case 'x':	// expression, only used for SETVAL
				// and even then, only with labels?
			if (!s->insn->val) {
				printf("?? (@%d)", __LINE__);
				break;
			}
			switch (s->insn->val->type) {
			case EXPR_LABEL:
				printf(".L%d", s->insn->val->symbol->bb_target->nr);
				break;
			default:
				printf("?? (@%d)", __LINE__);
			}
			break;

		case 'a':	// action/template from parent rule
			switch ((arg = *tmpl++)) {
				int rule;

			case '0':
				nt = rinfo->rhs[1];
			        rule = s->rules[nt];
				emit_tmpl(s, rule);
				break;
			case '1' ... ARG_MAX:
				k = s;
				do {
					nt = rinfo->rhs[arg-'1'];
					k = k->kids[arg-'1'];
					rule = k->rules[nt];
					arg = *tmpl++;
				} while (arg >= '1' && arg <= ARG_MAX);
				tmpl--;
				emit_tmpl(k, rule);
				break;
			}
			break;
		}
	}
}

static void emit_state(struct state *s, int rule)
{
	const struct rule_info *rinfo = &rules_info[rule];
	const char *tmpl = rinfo->action;

	if (!tmpl)
		return;

	// FIXME: should not happen?
	if (!rinfo->emit)		// It's a sub-rule
		return;			// do not emit anything

	//printf("# rule %ld\n", rinfo - &rules_info[0]);
	putchar('\t');
	emit_tmpl(s, rule);
	putchar('\n');
}


static void dump_state(FILE *f, const struct cg_state *cgs)
{
	const struct state *s = (const void*)cgs;
	struct instruction *insn = s->insn;
	struct position pos = {};
	int nr;
	int i;

	if (insn)
		pos = insn->pos;

	fprintf(f, "cg_state = %p, insn = %p:%s @ %d:%d\n", s, insn, name_opcode(s->op), pos.line, pos.pos);
	for (i = 0; i < NBR_KIDS; i++) {
		if (!s->kids[i])
			continue;
		fprintf(f, "\tkid %2d: %p\n", i, s->kids[i]);
	}
	nr = 0;
	for (i = 0; i < NBR_nterms; i++) {
		if (s->costs[i] == 65535 && s->rules[i] == 0)
			continue;
		fprintf(f, "\tnt  %2d: cost = %2u, rule = %3u\n", i, s->costs[i], s->rules[i]);
		nr++;
	}
	if (nr == 0)
		fprintf(f, "\t!! WARNING: no rules !!\n");
}

static void trace_tree_state(const struct cg_state *s, const char *end)
{
	printf("%s", name_opcode(s->op));
	if (s->insn)
		printf(".%d", s->insn->size);
	if (s->kids[0]) {
		printf("(");
		trace_tree_state(s->kids[0], "");
		if (s->kids[1]) {
			printf(", ");
			trace_tree_state(s->kids[1], "");
		}
		printf(")");
	} else if (s->op == INSN_CONST) {
		printf(" #%lld", s->src->value);
	} else if (s->op == INSN_REG) {
		printf(" %%r%d", s->src->nr);
	}
	printf("%s", end);
}

void trace_state(const char *msg, const struct cg_state *s)
{
	static int level;

	if (!TRACE)
		return;

	printf("%*s> %s:\n", level++, "", msg);
	trace_tree_state(s, "\n");
	dump_state(stdout, s);

	--level;
}


void reduce_state(struct cg_state *cgs, int nt)
{
	static int tlevel;
	struct state *s = (void*) cgs;
	int rule = s->rules[nt];
	const struct rule_info *rinfo = &rules_info[rule];
	int i;

	tlevel++;
	trace_reduce("(pre)\n");
	if (!rule) {
		dump_state(stderr, cgs);
		sparse_error(s->insn->pos, "%s(): can't reduce %s (nt = %d)\n", __func__, name_opcode(s->op), rule);
		trace_reduce(" ! ERROR !\n");
		trace_state("error norule", cgs);
		tlevel--;
		return;
	}

	trace_reduce("(ok)\n");
	for (i = 0; i < NBR_KIDS; i++) {
		if (!rinfo->rhs[i])
			break;
		trace_reduce("(kid %d)\n", i);
		reduce_state(cgs->kids[i], rinfo->rhs[i]);
	}
	trace_reduce("(kids)\n");
	if (rinfo->chain)
		reduce_state(cgs, rinfo->rhs[1]);
	trace_reduce("(chain)\n");

	trace_state("reduce", cgs);
	emit_state(s, rule);
	trace_reduce("(emit)\n\n");
	tlevel--;
}
