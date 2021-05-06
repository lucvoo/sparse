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


#ifndef	CODEGEN_H
#define	CODEGEN_H

#define	NBR_KIDS	3

struct term {
	const char	*name;
	int		arity;
};

struct ptree {
	const char	*name;
	int		count;
	int		size;
	int		arity;
	struct ptree	*kids[NBR_KIDS];
};

struct nterm {
	const char	*name;
	int		n;
	unsigned int	chain:1;	// is rhs of a chain rule
	unsigned int	used:1;		// check for unneeded rules
	unsigned int	defined:1;	// check for missing  rules

	struct nterm	*next;
};

struct rule {
	int		n;
	int		lineno;
	struct nterm	*lhs;

	const char	*name;		// rhs' name
	struct nterm	*nt;		// rhs' nterm  if  chain rule
	int		op;		// rhs' opcode if !chain rule
	int		arity;
	int		count;
	int		size;
	struct nterm	*kids[NBR_KIDS];

	int		cost;		// static cost
	unsigned int	emit:1;		// tmpl must be emitted vs. sub-pattern
	const char	*tmpl;

	struct rule	*next;
};


struct nterm *get_nterm(const char *name);
struct ptree *mktree(const char *name, int count, int size, struct ptree *left, struct ptree *right, struct ptree *extra);
void mkrule(int lineno, struct nterm *lhs, struct ptree *rhs, int cost, int emit, const char *tmpl);

int lookup_term(const char *name);
int check_term(unsigned int idx, int arity);
const char *opcode_name(unsigned int op);

int read_md(const char *path);

void die(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));

#endif
