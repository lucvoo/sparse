/*
 * memops - try to combine memory ops.
 *
 * Copyright (C) 2004 Linus Torvalds
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

#include "parse.h"
#include "expression.h"
#include "linearize.h"
#include "simplify.h"
#include "flow.h"
#include "ir.h"


static pseudo_t convert_to_phinode(struct basic_block *bb, struct instruction_list *dominators, struct symbol *type, struct ident *ident)
{
	struct instruction *node = alloc_phi_node(bb, type, ident);
	struct instruction *dom;

	add_phi_node(bb, node);

	// fill the 'arguments'
	FOR_EACH_PTR(dominators, dom) {
		struct instruction *phisrc = alloc_phisrc(dom->target, dom->type);
		insert_last_instruction(dom->bb, phisrc);
		link_phi(node, phisrc);
	} END_FOR_EACH_PTR(dom);
	return node->target;
}

static pseudo_t same_dominator(struct instruction_list *dominators)
{
	struct instruction *dom;
	pseudo_t new = NULL;

	/*
	 * Check for somewhat common case of duplicate
	 * phi nodes.
	 */
	FOR_EACH_PTR(dominators, dom) {
		if (!new)
			new = dom->target;
		else if (new != dom->target)
			return NULL;
	} END_FOR_EACH_PTR(dom);
	return new;
}

static pseudo_t find_dominating_parents(struct instruction *insn, struct basic_block *bb, int local)
{
	struct instruction_list *dominators = NULL;
	struct basic_block *parent;
	pseudo_t val;

loop:
	FOR_EACH_PTR(bb->parents, parent) {
		struct instruction *one;

		FOR_EACH_PTR_REVERSE(parent->insns, one) {
			int dominance;
			if (!one->bb)
				continue;
			dominance = dominates(insn, one, local);
			if (dominance < 0) {
				if (one->opcode == OP_LOAD)
					continue;
				return NULL;
			}
			if (dominance)
				goto found_dominator;
		} END_FOR_EACH_PTR_REVERSE(one);

		if (parent->generation == bb->generation)
			return NULL;
		parent->generation = bb->generation;

		if (bb_list_size(bb->parents) != 1)
			return NULL;
		bb = parent;
		goto loop;

found_dominator:
		add_instruction(&dominators, one);
	} END_FOR_EACH_PTR(parent);
	if (!dominators) {
		/* This happens with initial assignments to structures etc.. */
		if (!local)
			val = NULL;
		else
			val = value_pseudo(0);
	} else {
		val = same_dominator(dominators);
		if (!val)
			val = convert_to_phinode(bb, dominators, insn->type, insn->target->ident);
		free_ptr_list(&dominators);
	}
	return val;
}

static int address_taken(pseudo_t pseudo)
{
	struct pseudo_user *pu;
	FOR_EACH_PTR(pseudo->users, pu) {
		struct instruction *insn = pu->insn;
		if (insn->bb && (insn->opcode != OP_LOAD && insn->opcode != OP_STORE))
			return 1;
		if (pu->userp != &insn->src)
			return 1;
	} END_FOR_EACH_PTR(pu);
	return 0;
}

static int local_pseudo(pseudo_t pseudo)
{
	return pseudo->type == PSEUDO_SYM
		&& !(pseudo->sym->ctype.modifiers & (MOD_STATIC | MOD_NONLOCAL))
		&& !address_taken(pseudo);
}

static bool compatible_loads(struct instruction *a, struct instruction *b)
{
	if (is_integral_type(a->type) && is_float_type(b->type))
		return false;
	if (is_float_type(a->type) && is_integral_type(b->type))
		return false;
	return true;
}

static void rewrite_dominated_load(struct instruction *insn)
{
	struct basic_block *bb = insn->bb;
	pseudo_t pseudo = insn->src;
	int local = local_pseudo(pseudo);
	pseudo_t val;
	int changed = 0;

	bb->generation = ++bb_generation;
	val = find_dominating_parents(insn, bb, local);
	if (val) {
		changed = replace_with_pseudo(insn, val);
		repeat_phase |= changed;
	}
}

static void simplify_loads(struct basic_block *bb)
{
	struct instruction_list *worklist = NULL;
	struct instruction *insn;

	FOR_EACH_PTR_REVERSE(bb->insns, insn) {
		if (!insn->bb)
			continue;
		if (insn->opcode == OP_LOAD) {
			struct instruction *dom;
			pseudo_t pseudo = insn->src;
			int local = local_pseudo(pseudo);

			if (insn->is_volatile)
				continue;

			if (!has_users(insn->target)) {
				kill_instruction(insn);
				continue;
			}

			RECURSE_PTR_REVERSE(insn, dom) {
				int dominance;
				if (!dom->bb)
					continue;
				dominance = dominates(insn, dom, local);
				if (dominance) {
					/* possible partial dominance? */
					if (dominance < 0)  {
if (dominance == -3) {
replace_pseudo(insn, &insn->src, dom->target);
insn->opcode = OP_SLICE;
insn->from = bytes_to_bits(insn->offset - dom->offset);
insn->orig_type = dom->type;
goto next_load;
}
						if (dom->opcode == OP_LOAD)
							continue;
						goto next_load;
					}
					if (!compatible_loads(insn, dom))
						goto next_load;
					/* Yeehaa! Found one! */
					replace_with_pseudo(insn, dom->target);
					goto next_load;
				}
			} END_FOR_EACH_PTR_REVERSE(dom);

			/* OK, go find the parents */
			add_instruction(&worklist, insn);
		}
next_load:
		/* Do the next one */;
	} END_FOR_EACH_PTR_REVERSE(insn);

	FOR_EACH_PTR(worklist, insn) {
		rewrite_dominated_load(insn);
	} END_FOR_EACH_PTR(insn);
	free_ptr_list(&worklist);
}

static bool try_to_kill_store(struct instruction *insn,
			     struct instruction *dom, int local)
{
	int dominance = dominates(insn, dom, local);

	if (dominance) {
		/* possible partial dominance? */
		if (dominance == -3) {	// dom covers insn
			if (is_zero(dom->target) && is_zero(insn->target)) {
				kill_instruction_force(insn);
				goto next_store;
			}
		}
		if (dominance < 0)
			return false;
		if (insn->target == dom->target && insn->bb == dom->bb) {
			// found a memop which makes the store redundant
			kill_instruction_force(insn);
			return false;
		}
		if (dom->opcode == OP_LOAD)
			return false;
		if (dom->is_volatile)
			return false;
		/* Yeehaa! Found one! */
		kill_instruction_force(dom);
	}
	return true;
}

static void kill_dominated_stores(struct basic_block *bb)
{
	struct instruction *insn;

	FOR_EACH_PTR_REVERSE(bb->insns, insn) {
		if (!insn->bb)
			continue;
		if (insn->opcode == OP_STORE) {
			struct basic_block *par;
			struct instruction *dom;
			pseudo_t pseudo = insn->src;
			int local;

			if (!insn->type)
				continue;
			if (insn->is_volatile)
				continue;

			local = local_pseudo(pseudo);
			RECURSE_PTR_REVERSE(insn, dom) {
				if (!dom->bb)
					continue;
				if (!try_to_kill_store(insn, dom, local))
					goto next_store;
			} END_FOR_EACH_PTR_REVERSE(dom);

			/* OK, we should check the parents now */
			FOR_EACH_PTR(bb->parents, par) {

				if (bb_list_size(par->children) != 1)
					goto next_parent;
				FOR_EACH_PTR(par->insns, dom) {
					if (!dom->bb)
						continue;
					if (dom == insn)
						goto next_parent;
					if (!try_to_kill_store(insn, dom, local))
						goto next_parent;
				} END_FOR_EACH_PTR(dom);
next_parent:
				;
			} END_FOR_EACH_PTR(par);
		}
next_store:
		/* Do the next one */;
	} END_FOR_EACH_PTR_REVERSE(insn);
}

void simplify_memops(struct entrypoint *ep)
{
	struct basic_block *bb;
	pseudo_t pseudo;

	FOR_EACH_PTR_REVERSE(ep->bbs, bb) {
		simplify_loads(bb);
	} END_FOR_EACH_PTR_REVERSE(bb);

	FOR_EACH_PTR_REVERSE(ep->bbs, bb) {
		kill_dominated_stores(bb);
	} END_FOR_EACH_PTR_REVERSE(bb);

	FOR_EACH_PTR(ep->accesses, pseudo) {
		struct symbol *var = pseudo->sym;
		unsigned long mod;
		if (!var)
			continue;
		mod = var->ctype.modifiers;
		if (mod & (MOD_VOLATILE | MOD_NONLOCAL | MOD_STATIC))
			continue;
		kill_dead_stores(ep, pseudo, local_pseudo(pseudo));
	} END_FOR_EACH_PTR(pseudo);

	ir_validate_phi(ep);
}
