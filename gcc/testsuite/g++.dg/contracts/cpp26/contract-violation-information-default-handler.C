// Under -fcontract-violation-information=none the default violation handler
// prints no empty location parts and no empty comment: no " in function ",
// no " at :0", no ": ", and no message for a contract that had one -- for a
// precondition, one with a user message, a contract_assert, and a manual
// (P3290) violation with an empty comment, whose default location is empty
// too.
//
// Mirror, in the llvm_llvm-project fork:
// clang/test/Contracts/Runnable/contract-violation-information-default-handler.cpp
// (libc++'s handler has its own format); in the edgcpp_compiler fork,
// tests/tests/contracts/vinfo/default_handler.sft.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099 -fcontracts-p3290 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-violation-information=none" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

void pre_f (int x) pre (x > 0) { }
void msg_f (int x) pre (x > 0, "a user message") { }
void assert_f (int x) { contract_assert (x > 0); }

int
main ()
{
  pre_f (0);
  msg_f (0);
  assert_f (0);
  std::contracts::handle_observed_contract_violation ("");
}

// { dg-output "^contract violation(\n|\r\n|\r)\\\[assertion_kind: pre, semantic: observe, mode: predicate_false, terminating: no\\\](\n|\r\n|\r)" }
// { dg-output "contract violation(\n|\r\n|\r)\\\[assertion_kind: pre, semantic: observe, mode: predicate_false, terminating: no\\\](\n|\r\n|\r)" }
// { dg-output "contract violation(\n|\r\n|\r)\\\[assertion_kind: assert, semantic: observe, mode: predicate_false, terminating: no\\\](\n|\r\n|\r)" }
// { dg-output "contract violation(\n|\r\n|\r)\\\[assertion_kind: manual, semantic: observe, mode: unspecified, terminating: no\\\](\n|\r\n|\r)$" }
