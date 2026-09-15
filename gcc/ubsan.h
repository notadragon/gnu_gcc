/* UndefinedBehaviorSanitizer, undefined behavior detector.
   Copyright (C) 2013-2026 Free Software Foundation, Inc.
   Contributed by Marek Polacek <polacek@redhat.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_UBSAN_H
#define GCC_UBSAN_H

/* The various kinds of NULL pointer checks.  */
enum ubsan_null_ckind {
  UBSAN_LOAD_OF,
  UBSAN_STORE_OF,
  UBSAN_REF_BINDING,
  UBSAN_MEMBER_ACCESS,
  UBSAN_MEMBER_CALL,
  UBSAN_CTOR_CALL,
  UBSAN_DOWNCAST_POINTER,
  UBSAN_DOWNCAST_REFERENCE,
  UBSAN_UPCAST,
  UBSAN_CAST_TO_VBASE
};

/* This controls how ubsan prints types.  Used in ubsan_type_descriptor.  */
enum ubsan_print_style {
  UBSAN_PRINT_NORMAL,
  UBSAN_PRINT_POINTER,
  UBSAN_PRINT_ARRAY,
  UBSAN_PRINT_FORCE_INT
};

/* This controls ubsan_encode_value behavior.  */
enum ubsan_encode_value_phase {
  UBSAN_ENCODE_VALUE_GENERIC,
  UBSAN_ENCODE_VALUE_GIMPLE,
  UBSAN_ENCODE_VALUE_RTL
};

/* P3100 implicit contract assertions: the language-neutral reaction the
   middle end must emit for a core-language UB site, as resolved from the
   contract configuration by the LANG_HOOKS_RESOLVE_IMPLICIT_UB_SEMANTIC hook.
   Keeping this neutral lets ubsan.cc stay language-independent while the C++
   front end maps the P3595-resolved evaluation semantic onto it:

     assume                       -> IMPLICIT_UB_NONE     (no instrumentation)
     ignore                       -> IMPLICIT_UB_NONE      when the UB has no
                                     defined erroneous-behavior substitute (e.g.
                                     null deref -- raw operation); or
                                  -> IMPLICIT_UB_DEFINED   when it does (e.g.
                                     signed overflow -- instrument so the result
                                     is the defined wrapped value, no handler)
     quick_enforce                -> IMPLICIT_UB_TRAP      (__builtin_trap)
     noexcept_enforce             -> IMPLICIT_UB_NOEXCEPT_ENFORCE
     noexcept_observe             -> IMPLICIT_UB_NOEXCEPT_OBSERVE
                                     (call a nothrow __cxa_contract_violation
                                      entry point; the enforce entry decl is
                                      noreturn while the observe entry returns --
                                      no EH region needed)

   The enforce/observe distinction is kept in the reaction (rather than a single
   NOEXCEPT_HANDLER value) so that the specific semantic survives inlining: it is
   resolved once, pre-inline, with the correct enclosing-function context, and
   the handler-building langhook uses the carried reaction directly instead of
   re-resolving against the post-inline cfun->decl.

   Where a check does not support the potentially-throwing enforce/observe (e.g.
   the middle-end checks, whose sites run after EH lowering), those are excluded
   from its allowed set on the front-end side and clamped away, so they never
   reach the middle end as a reaction.  */
enum implicit_ub_reaction {
  IMPLICIT_UB_NONE = 0,
  IMPLICIT_UB_DEFINED,
  IMPLICIT_UB_TRAP,
  IMPLICIT_UB_NOEXCEPT_ENFORCE,
  IMPLICIT_UB_NOEXCEPT_OBSERVE
};

/* Operand positions of an IFN_UBSAN_NULL call.  A single .UBSAN_NULL carries
   two independent P3100 implicit-UB reactions -- one for the null-dereference
   edge (operands 3/4/5) and one for the misaligned-access edge (operands
   6/7/8) -- alongside the pointer, ckind and required alignment.  Every
   producer (c-ubsan.cc, ubsan.cc) emits all UBSAN_NULL_NUM_OPS operands, and
   every consumer (ubsan.cc, sanopt.cc, lto-streamer-in.cc) reads them by these
   names.  */
enum ubsan_null_op {
  UBSAN_NULL_PTR = 0,		/* the checked pointer  */
  UBSAN_NULL_CKIND,		/* ubsan_null_ckind + pointed-to type  */
  UBSAN_NULL_ALIGN,		/* required alignment, 0 if none  */
  UBSAN_NULL_REACTION,		/* null edge: implicit_ub_reaction  */
  UBSAN_NULL_ENTRY,		/* null edge: contract handler entry, or 0  */
  UBSAN_NULL_DATA,		/* null edge: contract data-block addr, or 0  */
  UBSAN_NULL_ALIGN_REACTION,	/* align edge: implicit_ub_reaction  */
  UBSAN_NULL_ALIGN_ENTRY,	/* align edge: contract handler entry, or 0  */
  UBSAN_NULL_ALIGN_DATA,	/* align edge: contract data-block addr, or 0  */
  UBSAN_NULL_NUM_OPS
};

extern bool ubsan_expand_bounds_ifn (gimple_stmt_iterator *);
extern bool ubsan_expand_null_ifn (gimple_stmt_iterator *);
extern bool ubsan_expand_objsize_ifn (gimple_stmt_iterator *);
extern bool ubsan_expand_ptr_ifn (gimple_stmt_iterator *);
extern bool ubsan_expand_vptr_ifn (gimple_stmt_iterator *);
extern bool ubsan_instrument_unreachable (gimple_stmt_iterator *);
extern tree ubsan_create_data (const char *, int, const location_t *, ...);
extern tree ubsan_type_descriptor (tree, ubsan_print_style
					 = UBSAN_PRINT_NORMAL);
extern tree ubsan_encode_value (tree, ubsan_encode_value_phase
				      = UBSAN_ENCODE_VALUE_GENERIC);
extern bool is_ubsan_builtin_p (tree);
extern tree ubsan_build_overflow_builtin (tree_code, location_t, tree, tree,
					  tree, tree *);
extern tree ubsan_instrument_float_cast (location_t, tree, tree);
extern tree ubsan_float_cast_overflow_predicate (tree, tree);
extern tree ubsan_get_source_location_type (void);
extern tree sanitize_unreachable_fn (tree *data, location_t loc);

#endif  /* GCC_UBSAN_H  */
