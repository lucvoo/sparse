// SPDX-License-Identifier: MIT
//
// This code is based on iburg:
// iburg is a code-generator generator that uses dynamic programming at
// compile time. It's described in
//
// C. W. Fraser, D. R. Hanson and T. A. Proebsting,
// Engineering a simple, efficient code generator generator,
// ACM Letters on Prog. Languages and Systems 1, 3 (Sep. 1992), 213-226.
// http://storage.webhop.net/documents/iburg.pdf
//
// and its code is available under the MIT license, see:
// https://github.com/drh/iburg

#include "codegen.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "insncode.h"
#include "list.h"


static struct rule *rules = NULL;
#define foreach_rule(r)		list_iterate(r, rules, next)

static void append_rule(struct rule *r)
{
	int n = list_append(rules, r, next);

	r->n = n + 1;			// rule 0 is invalid
}


static struct nterm *nterms = NULL;
static int n_nterms;
#define foreach_nterm(nt)	list_iterate(nt, nterms, next)

static void append_nterm(struct nterm *nt)
{
	list_append(nterms, nt, next);
	nt->n = n_nterms++;
}

static struct nterm *lookup_nterm(const char *name)
{
	struct nterm *nt;

	foreach_nterm(nt)
		if (strcmp(name, nt->name) == 0)
			return nt;

	return NULL;
}

struct nterm *get_nterm(const char *name)
{
	struct nterm *nt;

	nt = lookup_nterm(name);
	if (!nt) {
		nt = malloc(sizeof(*nt));
		append_nterm(nt);
		nt->name = name;
		nt->chain = 0;
		nt->used = 0;
		nt->defined = 0;
	}

	return nt;
}

static struct nterm *mk_internal_nterm(void)
{
	static int n;
	char *ptr;

	asprintf(&ptr, "_nt%d", n++);
	return get_nterm(ptr);
}

struct ptree *mktree(const char *name, int count, int size, struct ptree *left, struct ptree *right, struct ptree *extra)
{
	struct ptree *t;
	int arity = 0;

	if (left)
		arity++;
	if (right)
		arity++;
	if (extra)
		arity++;

	t = malloc(sizeof(*t));
	t->name = name;
	t->count = count;
	t->size = size;
	t->arity = arity;
	t->kids[0] = left;
	t->kids[1] = right;
	t->kids[2] = extra;

	return t;
}

static struct nterm *mkrule_internal(int lineno, struct ptree *t)
{
	struct nterm *nt;

	nt = mk_internal_nterm();
	mkrule(lineno, nt, t, 0, 0, NULL, NULL);
	return nt;
}

void mkrule(int lineno, struct nterm *lhs, struct ptree *rhs, int cost, int emit, const char *tmpl, const char *cond)
{
	struct rule *r = malloc(sizeof(*r));
	int i;

	// transform to a canonical rule, it's easier for the remaining
	for (i = 0; i < rhs->arity; i++) {
		struct nterm *nt;

		if (lookup_term(rhs->kids[i]->name) != -1) {
			nt = mkrule_internal(lineno, rhs->kids[i]);
		} else
			nt = get_nterm(rhs->kids[i]->name);

		r->kids[i] = nt;
	}

	// initialize the members
	r->lineno = lineno;
	r->lhs = lhs;
	r->name = rhs->name;
	r->arity = rhs->arity;
	r->count = rhs->count;
	r->size = rhs->size;
	r->cost = cost;
	r->emit = emit;
	r->tmpl = tmpl;
	r->cond = cond;

	append_rule(r);
}

static const char *show_count_suffix(const struct rule *r)
{
	switch (r->count) {
	case 1:	return "#B";
	case 2:	return "#H";
	case 3:	return "#L";
	case 4:	return "#Q";
	}

	return "";
}

static const char *show_size_suffix(const struct rule *r)
{
	switch (r->size) {
	case 1:	return ".B";
	case 2:	return ".H";
	case 3:	return ".L";
	case 4:	return ".Q";
	case 6:	return ".S";
	case 7:	return ".D";
	}

	return "";
}

static const char *show_rule(const struct rule *r)
{
	static char buf[256];
	size_t size = sizeof(buf);
	char *ptr = buf;
#define	rem	((size > (ptr - buf)) ? (size - (ptr - buf)) : 0)

	ptr += snprintf(ptr, rem, "%s: %s", r->lhs->name, r->name);
	if (r->count)
		ptr += snprintf(ptr, rem, "%s", show_count_suffix(r));
	if (r->size)
		ptr += snprintf(ptr, rem, "%s", show_size_suffix(r));
	if (r->kids[0])
		ptr += snprintf(ptr, rem, "(%s", r->kids[0]->name);
	if (r->kids[1])
		ptr += snprintf(ptr, rem, ", %s", r->kids[1]->name);
	if (r->kids[2])
		ptr += snprintf(ptr, rem, ", %s", r->kids[2]->name);
	if (r->kids[0])
		ptr += snprintf(ptr, rem, ")");
	snprintf(ptr, rem, "\t @ line %d", r->lineno);
#undef rem

	return buf;
}

static void generate_includes(const char *arch)
{
	printf("#include <stdio.h>\n");
	printf("#include <stdlib.h>\n");
	printf("#include <string.h>\n");
	printf("#include \"gencode.h\"\n");
	printf("#include \"codegen.h\"\n");
	printf("#include \"insncode.h\"\n");
	printf("#include \"lib.h\"\n");
	printf("\n");
}

static void generate_nterms(void)
{
	const struct nterm *nt;

	foreach_nterm(nt)
		printf("#define NT_%s\t%d\n", nt->name, nt->n);
	printf("\n");
	printf("#define NBR_nterms %d\n", n_nterms);
	printf("\n");
}

static void generate_opcodes(void)
{
	printf("static const char *opcodes[] = {\n");
	printf("#define INSN(O, N)      [INSN_ ## O] = #O,\n");
	printf("#define OP(O, N)        [INSN_ ## O] = #O,\n");
	printf("#include \"insncode.def\"\n");
	printf("#undef INSN\n");
	printf("#undef OP\n");
	printf("};\n");
	printf("\n");
	printf("static const char *name_opcode(unsigned int op)\n");
	printf("{\n");
	printf("\tif (op < (sizeof(opcodes)/sizeof(opcodes[0])))\n");
	printf("\t	return opcodes[op];\n");
	printf("\n");
	printf("\treturn NULL;\n");
	printf("}\n");
	printf("\n");

}

static void generate_rules(void)
{
	struct rule *r;

	printf("struct rule_info {\n");
	printf("\tunsigned short\trhs[NBR_KIDS];\n");
	printf("\tunsigned short\tsize:3;\n");
	printf("\tunsigned short\tcount:3;\n");
	printf("\tunsigned short\tchain:1;\n");
	printf("\tunsigned short\temit:1;\n");
	printf("\tconst char*\taction; // FIXME\n");
	printf("};\n\n");
	printf("static const struct rule_info rules_info[] = {\n");
	// FIXME: most of the rhs entries are the same
	//        should use pointer to the unique sub-arrays
	foreach_rule(r) {
		int i;

		printf("\t// %s\n", show_rule(r));
		printf("\t[%d] = {\n", r->n);
		printf("\t\t\t.rhs = { ");
		for (i = 0; i < NBR_KIDS && r->kids[i]; i++)
			printf("NT_%s, ", r->kids[i]->name);
		if (r->nt)		// is a chain rule
			printf("0, NT_%s, ", r->name);
		printf("},\n");
		if (r->tmpl)
			printf("\t\t\t.action = \"%s\",\n", r->tmpl);
		else
			printf("\t\t\t.action = NULL,\n");
		printf("\t\t\t.count = %d,\n", r->count);
		printf("\t\t\t.size = %d,\n", r->size);
		printf("\t\t\t.chain = %d,\n", r->nt ? 1 : 0);
		printf("\t\t\t.emit = %d,\n", r->emit);
		printf("\t},\n\n");
	}
	printf("};\n\n");
}

static void generate_struct_state(void)
{
	printf("struct state {\n");
	printf("	union {\n");
	printf("		struct {\n");
	printf("			enum insncode		op;\n");
	printf("			struct instruction	*insn;\n");
	printf("			pseudo_t		src;\n");
	printf("			struct state		*kids[NBR_KIDS];\n");
	printf("		};\n");
	printf("		struct cg_state			ext;\n");
	printf("	};\n");
	printf("	unsigned short	costs[NBR_nterms];\n");
	printf("	unsigned short	rules[NBR_nterms];\n");
	printf("};\n");
	printf("\n");
}

static void generate_chain_rule(const struct nterm *nt, const struct rule *r)
{
	const char *name;

	if (r->nt != nt)
		return;

	name = r->lhs->name;
	printf("\tc = cost + %d;\n", r->cost);
	printf("\tif (c < s->costs[NT_%s]) {\n", name);
	printf("\t\ts->costs[NT_%s] = c;\n", name);
	printf("\t\ts->rules[NT_%s] = %d;	// line %d\n", name, r->n, r->lineno);
	if (r->lhs->chain)
		printf("\n\t\tchain_rule_%s(s, c, rule);\n", r->lhs->name);
	printf("\t}\n");
}

static void generate_chain_rules(void)
{
	const struct nterm *nt;

	foreach_nterm(nt) {
		if (!nt->chain)
			continue;

		printf("static inline ");
		printf("void chain_rule_%s(struct state *s, int cost, int rule);\n", nt->name);
	}
	printf("\n");

	foreach_nterm(nt) {
		const struct rule *r;

		if (!nt->chain)
			continue;

		printf("static inline ");
		printf("void chain_rule_%s(struct state *s, int cost, int rule)\n", nt->name);
		printf("{\n");
		printf("\tunsigned c;\n");
		printf("\n");
		foreach_rule(r) {
			generate_chain_rule(nt, r);
		}
		printf("}\n");
		printf("\n");
	}
}

static void generate_base_rules(void)
{
	const struct nterm *nt;

	foreach_nterm(nt) {
		const char *name = nt->name;

		printf("static inline void base_rule_%s(struct state *s, int cost, int rule)\n", name);
		printf("{\n");
		printf("	if (cost >= s->costs[NT_%s])\n", name);
		printf("		return;\n");
		printf("\n");
		printf("	s->costs[NT_%s] = cost;\n", name);
		printf("	s->rules[NT_%s] = rule;\n", name);
		if (nt->chain)
			printf("\n	chain_rule_%s(s, cost, rule);\n", name);
		printf("}\n");
		printf("\n");
	}
}

static void print_condition(const struct rule *r)
{
	const char *str = r->cond;
	int c;

	while (1) {
		switch (c = *str++) {
		case 0:
			printf(" && ");
			return;
		case '%':
			c = *str++;
			switch (c) {
			case 'c':
				printf("s->src->value");
			}
			break;
		default:
			putchar(c);
		}
	}
}

static void generate_op_label(int op)
{
	//int i;
	int seen = 0;
	const struct rule *r;

	// search all the rules whose first terminal is op
	foreach_rule(r) {
		struct nterm *const *kids = r->kids;
		int count, size;

		if (r->nt || r->op != op)
			continue;
		if (!seen) {
			printf("	case INSN_%s:\n", opcode_name(op));
			seen = 1;
		}

		printf("\t\t// rule %d, line %d; action = %s\n", r->n, r->lineno, r->tmpl);
		printf("\t\t//   %s\n", show_rule(r));

		printf("\t\tif (");
		switch (r->arity) {
		case 0:
			break;
		case 1:
			printf("s0->rules[NT_%s] && ", kids[0]->name);
			break;
		case 2:
			printf("s0->rules[NT_%s] && ", kids[0]->name);
			printf("s1->rules[NT_%s] && ", kids[1]->name);
			break;
		case 3:
			printf("s0->rules[NT_%s] && ", kids[0]->name);
			printf("s1->rules[NT_%s] && ", kids[1]->name);
			printf("s2->rules[NT_%s] && ", kids[2]->name);
			break;
		}
		if ((count = r->count))
			printf("s1->src->value == %d && ", 4 << count);
		if ((size = r->size)) {
			if (size > 4)	// FP sizes
				size -= 3;
			printf("insn->size == %d && ", 4 << size);
		}
		if (r->cond)
			print_condition(r);
		printf("1) {\n");
		printf("\t\t\tcost = ");
		switch (r->arity) {
		case 0:
			break;
		case 1:
			printf("s0->costs[NT_%s] + ", kids[0]->name);
			break;
		case 2:
			printf("s0->costs[NT_%s] + ", kids[0]->name);
			printf("s1->costs[NT_%s] + ", kids[1]->name);
			break;
		case 3:
			printf("s0->costs[NT_%s] + ", kids[0]->name);
			printf("s1->costs[NT_%s] + ", kids[1]->name);
			printf("s2->costs[NT_%s] + ", kids[2]->name);
			break;
		}
		printf("%d;\n", r->cost);
		printf("\t\t\tbase_rule_%s(s, cost, %d);\n", r->lhs->name, r->n);
		printf("\t\t}\n");
	}

	if (seen) {
		printf("\t\tbreak;\n");
		printf("\n");
	}
}

static void generate_interface(void)
{
	int op;
	int i;

	printf("void trace_state(const char *msg, const struct cg_state *cgs);\n");
	printf("\n");
	printf("void label_state(struct cg_state *cgs)\n");
	printf("{\n");
	printf("\tstruct state *s = (void*)cgs;\n");
	for (i = 0; i< NBR_KIDS; i++)
		printf("\tstruct state *s%d = s->kids[%d];\n", i, i);
	printf("	struct instruction *insn = s->insn;\n");
	printf("	int cost;\n");
	printf("\n");
	//printf("printf(\"labeling(%%p:%%s)\\n\", s, name_opcode(s->op));\n");
	printf("	switch (s->op) {\n");
	for (op = 0; op < INSN_NBR; op++)
		generate_op_label(op);
	printf("	default:\n");
	printf("		sparse_error(insn->pos, \"%%s(): unknown terminal: %%s (%%d)\", __FUNCTION__, name_opcode(s->op), s->op);\n");
	printf("	}\n");
	printf("	trace_state(\"label\", cgs);\n");
	printf("}\n");
	printf("\n");
}


static int process_rules(void)
{
	struct rule *r;
	int err = 0;

	foreach_rule(r) {
		r->nt = lookup_nterm(r->name);
		if (r->nt) {		// is part of a rule chain
			r->nt->chain = 1;
		} else {
			r->op = lookup_term(r->name);
		}
	}

	// TODO: compress rules/nterms

	return err;
}


static void mark_nt(struct nterm *nt);

// Mark all the nterms of this rule as being used
static void mark_rule(struct rule *r)
{
	int i;

	mark_nt(r->nt);		// if a chain rule

	for (i = 0; i < NBR_KIDS; i++) {
		mark_nt(r->kids[i]);
	}
}

// Mark this nterm as used.
// Do the same for all the nterms in all rules defining this nt.
// If such a rule exist, mark the nt as being defined.
static void mark_nt(struct nterm *nt)
{
	struct rule *r;

	if (!nt)
		return;
	if (!nt || nt->used)
		return;

	nt->used = 1;
	foreach_rule(r) {

		if (r->lhs != nt)
			continue;

		mark_rule(r);
		nt->defined = 1;
	}
}

// Check that all the needed nterms exist.
// Also check that they are used (in itself,
// this is not  but can indicate a typo or something).
static int validate_nterms(void)
{
	struct nterm *nt;
	int err = 0;

	mark_nt(nterms);	// 'start' nterms

	foreach_nterm(nt) {
		const char *msg = NULL;
		int warning = 0;

		if (!nt->used) {
			msg = "is unneeded";
			warning = 1;
		} else if (!nt->defined) {
			msg = "is needed but never defined";
			err++;
		}

		if (msg) {
			const char *pre = warning ? "warning" : "error";
			fprintf(stderr, "%s: nterm '%s' %s.\n", pre, nt->name, msg);
		}
	}

	return err;
}

static int process_md(const char *mdfile)
{
	int err = 0;

	err += read_md(mdfile);
	err += process_rules();
	err += validate_nterms();
	return err;
}

static void generate_generator(const char *mdfile, const char *arch)
{
	generate_nterms();
	generate_opcodes();
	generate_rules();
	generate_struct_state();

	generate_chain_rules();
	generate_base_rules();
	generate_interface();

	printf("#include \"gen-common.c\"\n");
}


static void usage(const char *name)
{
	fprintf(stderr, "usage: %s arch-name MD-file\n", name);
	exit(1);
}

int main(int argc, const char *argv[])
{
	const char *mdfile, *arch;

	if (argc != 3)
		usage(argv[0]);
	arch   = argv[1];
	mdfile = argv[2];

	get_nterm("");		// lhs for top rules = 1st nterm

	printf("/* Automatically generated from %s */\n\n", mdfile);
	generate_includes(arch);
	printf("\n");
	printf("#include \"%s.c\"\n", arch);
	printf("\n");

	if (process_md(mdfile))
		exit(1);
	generate_generator(mdfile, arch);

	return 0;
}
