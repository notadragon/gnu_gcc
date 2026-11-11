// P3100: the routed sanitizer's case of
// g++.dg/contracts/cpp26/p3100-handler-effects-ipa.C: with -fsanitize=null
// routed to the contract-violation handler and the P3100 null check
// ignored (so the sanitizer's is made), the caller of a function whose
// only call is a member call through a null pointer of a member function
// that does nothing sees the handler's effects at -O2 (GCC-857).
// { dg-do run { target { i?86-*-linux* x86_64-*-linux* } } }
// { dg-additional-options "-std=c++26 -fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fsanitize=null -fsanitize-semantic=null:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "../contracts/cpp26/p3100-handler-effects-ipa.h"

// { dg-output "plain 1(\n|\r\n|\r)poly 1(\n|\r\n|\r)fin 1(\n|\r\n|\r)" }
// { dg-output "ptrnv 1(\n|\r\n|\r)" }
