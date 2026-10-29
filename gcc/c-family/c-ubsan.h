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

#ifndef GCC_C_UBSAN_H
#define GCC_C_UBSAN_H

/* P3100: set on the POINTER_PLUS_EXPR `p + i' the C++ front end builds as
   the address of the element p[i] of a subscript of the pointer p, where
   the null check of ub:expr.unary.dereference.nullptr applies: a null check
   of the element at a constant subscript checks p itself (DECISIONS.md E97
   (c), ubsan_p3100_constant_subscript).  */
#define P3100_SUBSCRIPT_ADDRESS_P(NODE) \
  TREE_LANG_FLAG_0 (POINTER_PLUS_EXPR_CHECK (NODE))

/* P3100: set on the POINTER_PLUS_EXPR `p + c' the C++ front end builds for
   a pointer plus or minus an integral constant expression, `p + c',
   `c + p', `p - c'; an indirection through it, `*(p + c)', marks it
   P3100_SUBSCRIPT_ADDRESS_P (DECISIONS.md E99 (a), E105 (a)).  */
#define P3100_ARITH_ADDRESS_P(NODE) \
  TREE_LANG_FLAG_1 (POINTER_PLUS_EXPR_CHECK (NODE))

/* P3100: set on the POINTER_PLUS_EXPR `p - offset' the C++ front end
   builds as the address of the derived object of a downcast of `*p' to a
   reference, `static_cast<D &> (*p)', with the base at a nonzero offset,
   where the null check of ub:expr.unary.dereference.nullptr is made under
   either leaf; also marked P3100_SUBSCRIPT_ADDRESS_P, so that a null
   check of the derived object checks p itself.  -fsanitize=null's check
   of a reference bound to it or a member function called on it checks p
   too, whatever the leaf (ubsan_maybe_instrument_reference_or_call), as
   the ubsan pass's check of a load through it does
   (p3100_constant_subscript_mem_ref).  */
#define P3100_DOWNCAST_ADDRESS_P(NODE) \
  TREE_LANG_FLAG_2 (POINTER_PLUS_EXPR_CHECK (NODE))

extern tree ubsan_instrument_division (location_t, tree, tree);
extern tree ubsan_instrument_shift (location_t, enum tree_code, tree, tree);
extern tree ubsan_instrument_vla (location_t, tree);
extern tree ubsan_instrument_return (location_t);
extern tree ubsan_instrument_bounds (location_t, tree, tree *, bool);
extern bool ubsan_array_ref_instrumented_p (const_tree);
extern void ubsan_maybe_instrument_array_ref (tree *, bool);
extern void ubsan_maybe_instrument_reference (tree *,
					      location_t = UNKNOWN_LOCATION);
extern tree ubsan_build_p3100_indirection_check (location_t, tree,
						 bool = false);
extern tree ubsan_p3100_base_offset_object (tree);
extern tree ubsan_p3100_check_base_offset_object (location_t, tree *);
extern tree ubsan_p3100_unguarded_sum (tree);
extern tree ubsan_p3100_subscript_pointer (tree);
extern tree ubsan_p3100_constant_subscript (tree, unsigned HOST_WIDE_INT);
extern tree ubsan_p3100_vptr_checked_downcast (tree);
extern bool ubsan_p3100_guarded_subscript_p (tree);
extern tree ubsan_p3100_rebuild_subscript (tree, tree, tree);
extern tree ubsan_maybe_instrument_member_call_object (location_t, tree,
						       bool = false);
extern void ubsan_maybe_instrument_member_call (tree, bool);
extern tree ubsan_maybe_instrument_ptrmem_object (location_t, tree);
extern void c_emit_contract_sanitizer_descriptors (void);
extern void c_apply_contract_sanitizer_assume (tree);

#endif  /* GCC_C_UBSAN_H  */
