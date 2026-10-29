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
   (uint16_t INTEGER_CST).  NULL_TREE means CES_ALL_ALLOWED_WITH_EXTENSIONS
   (the flag gates are applied when the mask is read).  */
#define CONTRACT_ALLOWED_MASK(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 7))

/* Group names from the label's group_names facet (TREE_LIST of
   STRING_CSTs), populated lazily by ensure_contract_groups.
   NULL_TREE = not yet extracted; error_mark_node = no groups.  */
#define CONTRACT_GROUPS(NODE) \
  (TREE_OPERAND (CONTRACT_CHECK (NODE), 8))

/* Group 3 -- Evaluation semantics (ops 9-11, lazy, NULL_TREE=unresolved).  */

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
   (dyn_configured << 8 | dyn_linkage << 1 | dyn_provideweak), where
   dyn_configured is the configuration entry's own semantic, which the
   selector's weak default returns.  */
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

/* Group 4 -- Postcondition-specific (ops 12-14, POSTCONDITION_STMT only).  */

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
						 tree = NULL_TREE,
						 char * = NULL);
extern tree build_contract_specifiers		(vec<tree, va_gc> *);
extern tree contract_specifiers_concat		(tree, tree);
extern void resolve_contract_label		(tree, tree, location_t);
extern void reresolve_contract_label_facets	(tree, tree, location_t);
extern tree finish_contract_condition		(cp_expr);
extern tree finish_contract_message_string	(tree, tree, location_t);
extern tree finish_contract_message		(tree, tree, tree, location_t);
extern bool maybe_define_contract_wrapper	(tree);
extern void update_late_contract		(tree, tree, tree, cp_expr,
						 char *);
extern void check_redecl_contract		(tree, tree);
extern void check_contract_on_defaulted_or_deleted (tree, bool);
extern tree invalidate_contract			(tree);
extern tree copy_and_remap_contracts		(tree, tree, contract_match_kind = cmk_all);
extern tree finish_contract_control_expr	(location_t, tree);
extern void diagnose_coroutine_postcondition_params (tree);
extern tree constify_contract_access		(tree);
extern bool constify_in_contract_predicate_p	(tree);
extern tree view_as_const			(tree);
extern tree contract_const_this		(tree);
extern bool contract_this_is_const_p		(void);

extern void set_fn_contract_specifiers		(tree, tree);
extern void update_fn_contract_specifiers	(tree, tree);
extern tree get_fn_contract_specifiers		(tree);
extern void remove_decl_with_fn_contracts_specifiers (tree);
extern void remove_fn_contract_specifiers	(tree);
extern void update_contract_arguments		(tree, tree);

extern tree make_postcondition_variable		(cp_expr);
extern tree make_postcondition_variable		(cp_expr, tree);
extern void check_postcondition_param_odr_uses	(tree, tree, location_t,
						 bool * = NULL);
extern void check_postcondition_object_odr_use	(tree);
extern bool postcondition_capture_init_ok_p	(tree, tree, location_t);
extern tree postcondition_capture_type	(tree *, bool, bool, location_t,
						 tree *);
extern bool contextless_postcondition_capture_p	(tree, tree *);
extern void member_template_spec_contracts	(tree, tree, bool, bool);
extern void check_postconditions_in_redecl	(tree, tree);
extern void check_postcondition_redecl_parm_types (tree, tree, tree);
extern void maybe_update_postconditions		(tree);
extern void rebuild_postconditions		(tree);
extern bool check_postcondition_result		(tree, tree, location_t);

/* P4405: a contract of a function as a caller-side evaluation of it at a
   call evaluates it, for the proof: for a specialization whose contracts
   are not yet substituted, a substituted copy; NULL_TREE if the function
   has no such contract; error_mark_node if the predicate cannot be
   described.  */
extern tree contract_for_caller_side_proof	(tree, tree);

/* P4405: the proof markers maybe_contract_wrap_call places with a call.  */
extern bool contract_proof_markers_p		(const_tree);
extern tree contract_strip_proof_markers	(tree);
extern tree contract_object_proof_markers	(tree);

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
extern tree get_orig_func_for_wrapper	(tree);

extern void start_function_contracts		(tree);
extern void maybe_apply_function_contracts	(tree);
struct override_shadows;
extern override_shadows *begin_override_shadows	(tree);
extern void start_override_shadows		(override_shadows *, tree);
extern void add_override_post_obligations	(tree);
extern void add_override_base_post_obligations	();
extern void place_override_shadows_alone	(override_shadows *);
extern void finish_function_outlined_contracts	(tree);
extern void set_contract_functions		(tree, tree, tree);

extern tree maybe_contract_wrap_call		(tree, tree,
						 bool = false);
extern bool emit_contract_wrapper_func		(bool);
extern void maybe_emit_violation_handler_wrappers (void);
extern bool contract_evaluation_settled_p	(tree, tree);
extern bool contract_specifiers_settled_p	(tree);
extern bool contract_wrapper_settled_p		(tree);

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

extern contract_evaluation_semantic get_constexpr_evaluation_semantic
  (const_tree);
extern contract_evaluation_semantic ensure_evaluation_semantic
  (tree, tree, bool);
/* D4405: CONTRACT_EVALUATION_SEMANTIC holds the runtime semantic in its low
   byte and the semantic of the analysis evaluation in the byte above;
   ensure_evaluation_semantic gives the first, and ensure_analysis_semantic
   both.  */
extern contract_evaluation_semantic ensure_analysis_semantic
  (tree, tree, contract_evaluation_semantic * = NULL);
/* D4405: diagnose taking the address of a function whose preconditions are
   analyzed at each call (cp_build_addr_expr_1).  */
extern void contract_diagnose_address_taken (tree, location_t,
					     tsubst_flags_t,
					     tree = NULL_TREE,
					     tree = NULL_TREE);
/* D4405: the definition of an object of static or thread storage duration
   takes the address of its destructor (expand_static_init), as does the
   creation of a temporary lifetime-extended to static or thread storage
   duration (set_up_extended_ref_temp).  */
extern void contract_diagnose_static_destructor (tree, tree = NULL_TREE,
						 location_t
						   = UNKNOWN_LOCATION);
/* True while build_addr_func forms the address a direct call calls.  */
extern bool contract_forming_direct_call;
/* The scope_chain while check_default_argument converts a default
   argument, which is diagnosed there, and while convert_default_arg
   converts it for a call, which is not; otherwise null.  */
extern struct saved_scope *contract_checking_default_argument;
extern struct saved_scope *contract_using_default_argument;
/* D4405: the same for converting a closure to a pointer to function
   (build_over_call).  */
extern void contract_diagnose_lambda_conversion (tree, location_t,
						 tsubst_flags_t);
/* D4405: while one is in scope (and ACTIVE), the calls built for the
   current function, or at namespace scope, are manifestly
   constant-evaluated, and have no analysis evaluation.  */
struct contract_constant_evaluation_sentinel
{
  explicit contract_constant_evaluation_sentinel (bool active = true);
  ~contract_constant_evaluation_sentinel ();
  bool saved_active;
  tree saved_fn;
  struct saved_scope *saved_chain;
};
/* D4405 (GCC-944): while one is in scope, the destructor calls built for
   the current function, or at namespace scope, are a temporary's cleanup
   (build_target_expr), analyzed only if the temporary is not elided:
   contract_drop_pending_analyses drops the analyses of an elided
   TARGET_EXPR's cleanup, and contract_resolve_pending_analysis analyzes
   a call that is made (cp_genericize_r, cp_gimplify_expr).  */
struct contract_temporary_cleanup_sentinel
{
  contract_temporary_cleanup_sentinel ();
  ~contract_temporary_cleanup_sentinel ();
  bool saved_active;
  tree saved_fn;
  struct saved_scope *saved_chain;
};
extern void contract_drop_pending_analyses (tree);
extern void contract_resolve_pending_analysis (tree);
/* D4405 (GCC-949): while one built with ACTIVE true is in scope, the
   analysis evaluations of the calls built for the current function are
   decided at once, not deferred to a proving strategy.  */
struct contract_no_proof_deferral_sentinel
{
  contract_no_proof_deferral_sentinel (bool active);
  ~contract_no_proof_deferral_sentinel ();
  bool saved_active;
  tree saved_fn;
  struct saved_scope *saved_chain;
};
/* D4405 (GCC-949): the proof markers of a partial-initialization
   cleanup's destructor call, for where the cleanup becomes active.  */
extern tree contract_potential_cleanup_markers (tree);
/* P3100: whether an implicit contract assertion guarding core-language UB may
   be built here.  Not in a template, whose instantiation builds it, and not
   in an unevaluated operand (noexcept/sizeof/decltype/requires), which
   generates no code -- and whose noexcept result an implicit assertion must
   never change, whatever its evaluation semantic.  */

inline bool
implicit_ub_check_context_p ()
{
  return (flag_contracts_p3100 && !processing_template_decl
	  && !cp_unevaluated_operand);
}

/* P3100 implicit contract assertions guarding core-language UB.  Each is
   documented where it is defined, in contracts.cc.  */
extern contract_evaluation_semantic resolve_implicit_contract_semantic
  (tree, location_t, const char *, uint16_t = CES_ALL_ALLOWED);
extern tree build_implicit_flow_off_check
  (tree, location_t, contract_evaluation_semantic);
extern tree build_implicit_coroutine_flow_off_check
  (tree, location_t, contract_evaluation_semantic);
extern tree build_implicit_divide_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
extern tree build_implicit_float_divide_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
extern tree build_implicit_shift_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
extern tree build_implicit_divide_overflow_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
extern tree build_implicit_float_cast_check
  (tree, location_t, contract_evaluation_semantic, tree, tree);
extern tree build_implicit_enum_cast_check
  (tree, location_t, contract_evaluation_semantic, tree, tree, tree);
extern tree build_implicit_pure_virtual_direct_check
  (location_t, tree);
extern tree build_implicit_pure_virtual_terminus (tree);
extern bool contract_constexpr_ignored_p (const_tree);
extern bool contract_constexpr_terminating_p (const_tree);

/* In pt.cc.  Substitute a template instantiation's contracts on demand, for a
   reader other than its own definition -- a P3097 wrapper or a P3595
   caller-side check.  A function that is never defined would otherwise never
   have them substituted at all.  */
extern void maybe_instantiate_contracts (tree, bool = false);

/* Contract data-block ABI constants.

   This is a mirror of libcontracts/contracts-abi.h, which is the source of
   truth.  It cannot be included here: libcontracts is a target library built
   as a separate unit, and the compiler must not depend on its headers.  So the
   values are restated, and changing one side means changing both.

   The names are what makes the mirror visible.  Spelled as bare hex in the
   descriptor-table initializers -- `0x01`, `0x02`, ... -- these values would
   have nothing to grep for on the compiler side while the runtime names every
   one of them, so a renumbering there would leave no trace here.  It is a
   mirror, not a sync: nothing mechanically checks the two agree.  */

/* Field ids in a contract data block.  __cxa_field_id_t.  */
enum cxa_field_id
{
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
enum cxa_vendor_id
{
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
