// P3100: with -fsanitize=shift, signed-integer-overflow and
// integer-divide-by-zero routed to the contract-violation handler with
// noexcept_observe, the code after a check that reports and continues
// sees the handler's side effects, in the function holding the check and
// in its caller (GCC-858): a routed check is neither a leaf call (the
// __ubsan_handle_* functions, declared leaf, flags_from_decl_or_type) nor a
// const one (IFN_UBSAN_CHECK_*, gimple_call_flags).  The forms are in
// p3100-routed-handler-effects-builtin.h.
// At -O2.
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=shift,signed-integer-overflow,integer-divide-by-zero" }
// { dg-additional-options "-fsanitize-semantic=shift:noexcept_observe,signed-integer-overflow:noexcept_observe,integer-divide-by-zero:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-routed-handler-effects-builtin.h"

// { dg-output "shl 1(\n|\r\n|\r)add 1(\n|\r\n|\r)sub 1(\n|\r\n|\r)" }
// { dg-output "mul 1(\n|\r\n|\r)neg 1(\n|\r\n|\r)dvo 1(\n|\r\n|\r)" }
// { dg-output "dvz 1(\n|\r\n|\r)ishl 1(\n|\r\n|\r)iadd 1(\n|\r\n|\r)" }
// { dg-output "isub 1(\n|\r\n|\r)imul 1(\n|\r\n|\r)ineg 1(\n|\r\n|\r)" }
// { dg-output "idvo 1(\n|\r\n|\r)idvz 1(\n|\r\n|\r)" }
