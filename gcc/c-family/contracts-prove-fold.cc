/* The folding proving strategy for the P4405 prove and diagnose evaluation
   semantics (-fcontracts-prove-fold).  See contracts-prove.h.

   Copyright (C) 2026 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* A predicate the front end folds to true is proven; one it folds to false
   is always false.  Folding assumes nothing but the language's own rules
   -- including that no undefined behavior happens, which has assume's
   license -- so every proof is sound.  The fold works on a copy and the
   predicate is never evaluated at run time.

   For a caller-side evaluation of a precondition, the predicate is folded
   with the call's arguments substituted for the callee's parameters: a
   precondition sees each parameter as initialized from its argument, so an
   argument that folds to a constant is the parameter's value.  Only that
   case is substituted -- a by-value parameter of scalar type whose argument
   folds to a constant without side effects, or to the address of an object
   with static storage duration.  A reference parameter, one of class type,
   and a non-constant argument are left as they are, as is everything when
   the predicate contains a statement (a statement expression, a
   declaration) or a tree the copy cannot see into (a constructor, a
   front-end-specific node).  An unsubstituted parameter is an unknown
   value, so a fold that still yields true or false is still sound.  An
   argument is folded and never copied into anything that is evaluated, so
   one with side effects is harmless.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "fold-const.h"
#include "options.h"
#include "c-family/contracts-prove.h"

/* The value of a parameter of type TYPE initialized from ARG, if the fold
   may substitute it for the parameter, else NULL_TREE.  */

static tree
fold_parameter_value (tree type, tree arg)
{
  if (!(INTEGRAL_TYPE_P (type)
	|| SCALAR_FLOAT_TYPE_P (type)
	|| TREE_CODE (type) == POINTER_TYPE
	|| TREE_CODE (type) == NULLPTR_TYPE))
    return NULL_TREE;
  tree v = c_contract_fold_predicate (arg);
  if (v == error_mark_node || TREE_SIDE_EFFECTS (v))
    return NULL_TREE;
  tree stripped = tree_strip_nop_conversions (v);
  if (!(CONSTANT_CLASS_P (stripped)
	|| (TREE_CODE (stripped) == ADDR_EXPR && TREE_CONSTANT (stripped))))
    return NULL_TREE;
  return fold_convert (type, v);
}

/* walk_tree callback copying a predicate while replacing the parameters
   mapped in DATA, a hash_map<tree, tree>.  Only expression nodes are copied
   and entered; declarations, types and constants are shared and not
   entered.  Any other node stops the walk and the substitution is
   abandoned: a statement, whose walk enters its declarations' initializers;
   a list, vector or constructor, whose elements a node copy would share;
   and any front-end-specific node, whose walk the front end owns.  */

static tree
fold_substitute_r (tree *tp, int *walk_subtrees, void *data)
{
  tree t = *tp;
  enum tree_code code = TREE_CODE (t);
  if (code == PARM_DECL)
    {
      if (tree *v = static_cast<hash_map<tree, tree> *> (data)->get (t))
	*tp = unshare_expr (*v);
      *walk_subtrees = 0;
      return NULL_TREE;
    }
  if (code == BIND_EXPR || code == DECL_EXPR
      || (int) code >= (int) LAST_AND_UNUSED_TREE_CODE)
    return t;
  if (IS_EXPR_CODE_CLASS (TREE_CODE_CLASS (code)))
    {
      *tp = copy_node (t);
      return NULL_TREE;
    }
  if (TREE_CODE_CLASS (code) == tcc_exceptional)
    return t;
  /* A declaration other than a parameter, a type or a constant.  */
  *walk_subtrees = 0;
  return NULL_TREE;
}

/* QUERY's predicate with the arguments of its call substituted for its
   parameters where that is sound (see the comment at the top of the file),
   or the predicate itself when nothing is substituted.  */

static tree
fold_substitute_arguments (const contract_proof_query &query)
{
  if (!query.call || TREE_CODE (query.call) != CALL_EXPR)
    return query.predicate;

  hash_map<tree, tree> map;
  int nargs = call_expr_nargs (query.call);
  int i = 0;
  for (tree p = query.parms; p && i < nargs; p = DECL_CHAIN (p), ++i)
    if (tree v = fold_parameter_value (TREE_TYPE (p),
				       CALL_EXPR_ARG (query.call, i)))
      map.put (p, v);
  if (map.is_empty ())
    return query.predicate;

  tree copy = query.predicate;
  if (walk_tree (&copy, fold_substitute_r, &map, NULL))
    return query.predicate;
  return copy;
}

static bool
fold_enabled_p ()
{
  return flag_contracts_prove_fold;
}

static contract_proof_result
fold_prove (const contract_proof_query &query)
{
  tree pred = fold_substitute_arguments (query);
  tree folded = c_contract_fold_predicate (pred);
  if (integer_onep (folded))
    return CPR_SOUND;
  if (integer_zerop (folded))
    return CPR_ALWAYS_FALSE;
  return CPR_UNPROVEN;
}

const contract_proof_strategy contract_prove_fold_strategy = {
  "fold",
  /*may_depend_on_assumptions=*/false,
  fold_enabled_p,
  fold_prove,
  /*defers_p=*/NULL,
  /*mark=*/NULL,
  /*assume=*/NULL,
  /*take_diagnostic=*/NULL,
  /*note_premise=*/NULL
};
