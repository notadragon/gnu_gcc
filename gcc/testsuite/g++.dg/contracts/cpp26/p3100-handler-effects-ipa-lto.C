// P3100: the caller of a function whose only call is a member call through
// a null pointer of a member function that does nothing (const) sees the
// effects of the contract-violation handler the function's null check calls
// when optimizing (GCC-857): such a check is a call of user code with no
// call graph edge until it is expanded, so ipa-reference takes the
// function to read and write any static, and `reports', which the handler
// alone changes, is read again after the call.  The call through a pointer
// to member function, an indirect call, is the control.  The forms are in
// p3100-handler-effects-ipa.h.
// With -flto, where ipa-reference runs at link time.
// { dg-do run { target c++26 } }
// { dg-require-effective-target lto }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2 -flto" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-handler-effects-ipa.h"

// { dg-output "plain 1(\n|\r\n|\r)poly 1(\n|\r\n|\r)fin 1(\n|\r\n|\r)" }
// { dg-output "ptrnv 1(\n|\r\n|\r)" }
