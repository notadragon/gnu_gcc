/* Definitions for C++26 contracts.

   Copyright (C) 2020-2026 Free Software Foundation, Inc.
   Originally by Jeff Chapman II (jchapman@lock3software.com) for proposed
   C++20 contracts.
   Rewritten for C++26 contracts by:
     Nina Ranns (dinka.ranns@googlemail.com)
     Iain Sandoe (iain@sandoe.co.uk)
     Ville Voutilainen (ville.voutilainen@gmail.com).

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

#ifndef GCC_CP_CONTRACT_H
#define GCC_CP_CONTRACT_H

#include <cstdint>
#include "c-family/contracts-config.h"

enum detection_mode : uint16_t {
  CDM_UNSPECIFIED = 0,
  CDM_PREDICATE_FALSE = 1,
  CDM_EVAL_EXCEPTION = 2
};

#define CONTRACT_CHECK(NODE) \
  (TREE_CHECK3 (NODE, ASSERTION_STMT, PRECONDITION_STMT, POSTCONDITION_STMT))

/* Group 1 -- Structural (ops 0-6, immutable after parse).  */

/* The assertion kind (CAK_PRE, CAK_POST, CAK_ASSERT).  */
#define CONTRACT_ASSERTION_KIND(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 0))

/* The parsed condition of the contract.  */
#define CONTRACT_CONDITION(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 1))

/* The raw comment of the contract.  */
#define CONTRACT_COMMENT(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 2))

/* A std::source_location, if provided.  */
#define CONTRACT_STD_SOURCE_LOC(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 3))

/* The user-defined diagnostic message (P3099), or NULL_TREE if none.  */
#define CONTRACT_MESSAGE(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 4))

/* The assertion-control label (P3400), or NULL_TREE if none.  */
#define CONTRACT_LABEL(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 5))

/* The requires-clause constraint (P4283), or NULL_TREE if none.  */
#define CONTRACT_REQUIRES_CLAUSE(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 6))

/* Group 2 -- Config resolution inputs (ops 7-8, eager, set at parse time).  */

/* Bitmask of semantics allowed by the label's allowed_semantics facet
   (uint16_t INTEGER_CST).  NULL_TREE means CES_ALL_ALLOWED.  */
#define CONTRACT_ALLOWED_MASK(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 7))

/* Group names from the label's group_names facet (TREE_LIST of
   STRING_CSTs), populated lazily by fill_query_groups.
   NULL_TREE = not yet extracted; error_mark_node = no groups.  */
#define CONTRACT_GROUPS(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 8))

/* Group 3 -- Evaluation semantics (ops 9-10, lazy, NULL_TREE=unresolved).  */

/* The runtime callee-side evaluation semantic.  */
#define CONTRACT_EVALUATION_SEMANTIC(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 9))

/* The constexpr callee-side evaluation semantic.  */
#define CONTRACT_CONSTEXPR_EVALUATION_SEMANTIC(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 10))

/* The runtime dynamic-selector descriptor (P3595 output.dynamic), set
   lazily by ensure_evaluation_semantic when !in_ce.  NULL_TREE means the
   contract has no dynamic selection.  When present it is a TREE_LIST whose
   TREE_PURPOSE is an IDENTIFIER_NODE (the selector name) and whose
   TREE_VALUE is an INTEGER_CST packing
   (dyn_linkage << 1 | dyn_provideweak).  */
#define CONTRACT_DYNAMIC(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 11))

/* True if NODE is any kind of contract.  */
#define CONTRACT_P(NODE)			\
  (TREE_CODE (NODE) == ASSERTION_STMT		\
   || TREE_CODE (NODE) == PRECONDITION_STMT	\
   || TREE_CODE (NODE) == POSTCONDITION_STMT)

/* True if NODE is a contract condition.  */
#define CONTRACT_CONDITION_P(NODE)		\
  (TREE_CODE (NODE) == PRECONDITION_STMT	\
   || TREE_CODE (NODE) == POSTCONDITION_STMT)

/* True if NODE is a precondition.  */
#define PRECONDITION_P(NODE)           \
  (TREE_CODE (NODE) == PRECONDITION_STMT)

/* True if NODE is a postcondition.  */
#define POSTCONDITION_P(NODE)          \
  (TREE_CODE (NODE) == POSTCONDITION_STMT)

/* The contract specifiers of a function are held in a TREE_VEC, each element
   of which is a PRECONDITION_STMT or a POSTCONDITION_STMT.  A function with
   no contracts has NULL_TREE rather than an empty vector.  */

/* True iff the FUNCTION_DECL NODE currently has any contracts.  */
#define DECL_HAS_CONTRACTS_P(NODE) \
  (get_fn_contract_specifiers (NODE) != NULL_TREE)

/* True iff the condition of the contract NODE is not yet parsed.  */
#define CONTRACT_CONDITION_DEFERRED_P(NODE) \
  (TREE_CODE (CONTRACT_CONDITION (NODE)) == DEFERRED_PARSE)

/* Group 4 -- Postcondition-specific (ops 11-12, POSTCONDITION_STMT only).  */

/* The VAR_DECL of a postcondition result.  For deferred contracts, this
   is an IDENTIFIER.  */
#define POSTCONDITION_IDENTIFIER(NODE) \
  (TREE_OPERAND (POSTCONDITION_STMT_CHECK (NODE), 12))

/* The postcondition captures -- a TREE_LIST of capture VAR_DECLs with
   DECL_INITIAL set to the initializer expression, or NULL_TREE if the
   postcondition has no captures (P3098).  */
#define POSTCONDITION_CAPTURES(NODE) \
  (TREE_OPERAND (POSTCONDITION_STMT_CHECK (NODE), 13))

/* The attribute-specifier-seq written on the result-name-introducer, or
   NULL_TREE.  The predicate is always parsed late -- the seq belongs to the
   complete declarator, and at that point the function's parameters are no
   longer in scope -- so the attributes cannot be applied where they are
   read, and travel here until the result VAR_DECL is built.  */
#define POSTCONDITION_RESULT_ATTRS(NODE) \
  (TREE_OPERAND (POSTCONDITION_STMT_CHECK (NODE), 14))

/* For a FUNCTION_DECL of a guarded function, this holds the function decl
   where pre contract checks are emitted.  */
#define DECL_PRE_FN(NODE) \
  (get_precondition_function ((NODE)))

/* For a FUNCTION_DECL of a guarded function, this holds the function decl
   where post contract checks are emitted.  */
#define DECL_POST_FN(NODE) \
  (get_postcondition_function ((NODE)))

/* True iff the FUNCTION_DECL is the pre function for a guarded function.  */
#define DECL_IS_PRE_FN_P(NODE) \
  (DECL_DECLARES_FUNCTION_P (NODE) && DECL_LANG_SPECIFIC (NODE) \
   && CONTRACT_HELPER (NODE) == ldf_contract_pre)

/* True iff the FUNCTION_DECL is the post function for a guarded function.  */
#define DECL_IS_POST_FN_P(NODE) \
  (DECL_DECLARES_FUNCTION_P (NODE) && DECL_LANG_SPECIFIC (NODE) \
   && CONTRACT_HELPER (NODE) == ldf_contract_post)

#define DECL_IS_WRAPPER_FN_P(NODE) \
  (DECL_DECLARES_FUNCTION_P (NODE) && DECL_LANG_SPECIFIC (NODE) && \
   DECL_CONTRACT_WRAPPER (NODE))

/* Allow specifying a sub-set of contract kinds to copy.  */
enum contract_match_kind
{
  cmk_all,
  cmk_pre,
  cmk_post
};

/* contracts.cc */

extern void init_contracts			(void);

extern tree grok_contract			(tree, tree, cp_expr,
						 location_t,
						 tree = NULL_TREE,
						 tree = NULL_TREE,
						 tree = NULL_TREE);
extern tree build_contract_specifiers		(vec<tree, va_gc> *);
extern tree contract_specifiers_concat		(tree, tree);
extern void resolve_contract_label		(tree, tree, location_t);
extern void reresolve_contract_label_facets (tree, tree, location_t);
extern tree finish_contract_condition		(cp_expr);
extern tree finish_contract_message		(tree, tree, tree, location_t);
extern bool maybe_define_contract_wrapper	(tree);
extern void update_late_contract		(tree, tree, tree, cp_expr);
extern void check_redecl_contract		(tree, tree);
extern void check_contract_on_defaulted_or_deleted (tree, bool);
extern tree invalidate_contract			(tree);
extern tree copy_and_remap_contracts		(tree, tree, contract_match_kind = cmk_all);
extern void diagnose_coroutine_postcondition_params (tree);
extern tree constify_contract_access		(tree);
extern bool constify_in_contract_predicate_p	(tree);
extern tree view_as_const			(tree);

extern void set_fn_contract_specifiers		(tree, tree);
extern void update_fn_contract_specifiers	(tree, tree);
extern tree get_fn_contract_specifiers		(tree);
extern void remove_decl_with_fn_contracts_specifiers (tree);
extern void remove_fn_contract_specifiers	(tree);
extern void update_contract_arguments		(tree, tree);

extern tree make_postcondition_variable		(cp_expr);
extern tree make_postcondition_variable		(cp_expr, tree);
extern void check_postcondition_param_odr_uses	(tree, tree, location_t);
extern void check_postconditions_in_redecl	(tree, tree);
extern void check_postcondition_redecl_parm_types (tree, tree, tree);
extern void maybe_update_postconditions		(tree);
extern void rebuild_postconditions		(tree);
extern bool check_postcondition_result		(tree, tree, location_t);

extern bool contract_any_deferred_p 		(tree);
/* Late-parse the deferred predicates of a redeclaration's CONTRACTS as if
   they belonged to FNDECL.  Supplied by the parser, which owns the token
   caches, and called only from flush_deferred_contract_matches.  */
typedef void (*late_contract_parse_fn) (tree, tree, tree);
extern void flush_deferred_contract_matches (late_contract_parse_fn);
/* Late-parse FNDECL's own deferred contract predicates.  Defined in the
   parser, which owns the token caches.  */
extern void cp_late_parse_function_contracts (tree);

extern tree get_precondition_function		(tree);
extern tree get_postcondition_function		(tree);
extern tree get_orig_for_outlined		(tree);

extern void start_function_contracts		(tree);
extern void maybe_apply_function_contracts	(tree);
extern void finish_function_outlined_contracts	(tree);
extern void set_contract_functions		(tree, tree, tree);

extern tree maybe_contract_wrap_call		(tree, tree,
						 bool = false);
extern bool emit_contract_wrapper_func		(bool);
extern void maybe_emit_violation_handler_wrappers (void);

extern tree build_contract_check		(tree);
extern void check_handle_contract_violation	(tree);

/* Test if EXP is a contract const wrapper node.  */

inline bool
contract_const_wrapper_p (const_tree exp)
{
  /* A wrapper node has code VIEW_CONVERT_EXPR, and the flag base.private_flag
     is set. The wrapper node is used to used to constify entities inside
     contract assertions.  */
  return ((TREE_CODE (exp) == VIEW_CONVERT_EXPR) && CONST_WRAPPER_P (exp));
}

/* If EXP is a contract_const_wrapper_p, return the wrapped expression.
   Otherwise, do nothing. */

inline tree
strip_contract_const_wrapper (tree exp)
{
  if (contract_const_wrapper_p (exp))
    return TREE_OPERAND (exp, 0);
  else
    return exp;
}

extern contract_evaluation_semantic get_evaluation_semantic (const_tree);
extern contract_evaluation_semantic get_constexpr_evaluation_semantic
  (const_tree);
extern contract_evaluation_semantic ensure_evaluation_semantic
  (tree, tree, bool);
/* P3100: resolve the evaluation semantic for a synthesized implicit contract
   assertion guarding core-language UB (UB_ID names the P3100 identifier /
   config group).  ALLOWED is the base set of C++26 semantics this kind of
   check can emit (a subset of CES_ALL_ALLOWED); "assume" is always added, and
   the P4298 noexcept variants are added under -fcontracts-p4298.  A configured
   semantic outside the resulting set is clamped via the resolution fallback
   order.  */
extern contract_evaluation_semantic resolve_implicit_contract_semantic
  (tree, location_t, const char *, uint16_t = CES_ALL_ALLOWED);
/* P3100: build the GENERIC reaction to append at a value-returning function's
   fall-off point for the resolved semantic SEM (NULL_TREE for assume /
   none).  */
extern tree build_implicit_flow_off_check
  (tree, location_t, contract_evaluation_semantic);
/* P3100: reaction for control flowing off the end of a coroutine with no
   return_void ({stmt.return.coroutine.flow.off}); NULL_TREE for
   assume/ignore.  */
extern tree build_implicit_divide_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
/* P3100: build the guarded replacement for an integer shift SHIFT_RESULT (OP0
   shifted by OP1) whose shift amount may be out of range, for semantic SEM.  */
extern tree build_implicit_shift_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
/* P3100: build the guarded replacement for a signed division/remainder
   DIV_RESULT whose quotient (OP0 / OP1) is not representable (OP0 == MIN,
   OP1 == -1), for semantic SEM.  */
extern tree build_implicit_divide_overflow_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
/* P3100: build the guarded replacement for a floating-point-to-integer
   conversion CONVERTED (of the single-evaluation floating value EXPR) whose
   truncated value may not be representable in the destination integer type, for
   semantic SEM ({conv.fpint}).  */
extern tree build_implicit_float_cast_check
  (tree, location_t, contract_evaluation_semantic, tree, tree);
/* P3100: build the guarded replacement for an integer/enumeration ->
   enumeration conversion CONVERTED (of the single-evaluation source value EXPR)
   whose value may be outside the target enumeration ENUMTYPE's value range, for
   a non-fixed-underlying-type enum, for semantic SEM
   ({expr.static.cast.enum.outside.range}).  */
extern tree build_implicit_pure_virtual_terminus (tree);
extern bool contract_constexpr_ignored_p (const_tree);
extern bool contract_constexpr_terminating_p (const_tree);

/* In pt.cc.  Substitute a template instantiation's contracts on demand, for a
   reader other than its own definition -- a P3097 wrapper or a P3595
   caller-side check.  A function that is never defined would otherwise never
   have them substituted at all.  */
extern void maybe_instantiate_contracts (tree);

/* Will this contract be ignored.  */

inline bool
contract_ignored_p (const_tree contract)
{
  return (get_evaluation_semantic (contract) <= CES_IGNORE);
}

/* Will this contract be evaluated?  */

inline bool
contract_evaluated_p (const_tree contract)
{
  return (get_evaluation_semantic (contract) >= CES_OBSERVE);
}

/* Is the contract terminating?  */

inline bool
contract_terminating_p (const_tree contract)
{
  return (get_evaluation_semantic (contract) == CES_ENFORCE
	  || get_evaluation_semantic (contract) == CES_QUICK
	  || get_evaluation_semantic (contract) == CES_NOEXCEPT_ENFORCE);
}

/* Contract data-block ABI constants.

   THIS IS A MIRROR of libcontracts/contracts-abi.h, which is the source of
   truth.  It cannot be included here: libcontracts is a target library built
   as a separate unit, and the compiler must not depend on its headers.  So the
   values are restated, and changing one side means changing both.

   The names are what makes the mirror visible.  Spelled as bare hex in the
   descriptor-table initializers -- `0x01`, `0x02`, ... -- these values would
   have nothing to grep for on the compiler side while the runtime names every
   one of them, so a renumbering there would leave no trace here.  It is a
   mirror, not a sync: nothing mechanically checks the two agree.  */

/* Field ids in a contract data block.  __cxa_field_id_t.  */
enum cxa_field_id {
  CXA_FIELD_SOURCE_LOCATION	= 0x01,
  CXA_FIELD_COMMENT		= 0x02,
  CXA_FIELD_MESSAGE		= 0x03,
  CXA_FIELD_LOCAL_HANDLER	= 0x04,
  CXA_FIELD_QUERY_FUNCTION	= 0x05,
  CXA_FIELD_LABEL_PTR		= 0x06,
  CXA_FIELD_ASSERTION_KIND	= 0x07,
  CXA_FIELD_EVALUATION_SEMANTIC = 0x08,
  CXA_FIELD_DETECTION_MODE	= 0x09,
  CXA_FIELD_EXCEPTION_PTR	= 0x0A,
  CXA_FIELD_REPORT		= 0x0B,
  CXA_FIELD_EXTENDED		= 0x40
};

/* Producer ids.  __cxa_vendor_id_t.  */
enum cxa_vendor_id {
  CXA_VENDOR_GENERIC = 0x0,
  CXA_VENDOR_GCC     = 0x1,
  CXA_VENDOR_CLANG   = 0x2,
  CXA_VENDOR_MSVC    = 0x3
};

/* The version nibble of a descriptor-table header byte.  */
#define CXA_DESC_VERSION 1

/* The header byte of a descriptor table: version in the high nibble, producer
   in the low one.  */
#define CXA_DESC_HEADER_BYTE \
  ((CXA_DESC_VERSION << 4) | CXA_VENDOR_GCC)

#endif /* ! GCC_CP_CONTRACT_H */
