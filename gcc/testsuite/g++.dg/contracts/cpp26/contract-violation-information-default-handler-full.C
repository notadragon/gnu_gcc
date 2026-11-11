// Under -fcontract-violation-information=full the default violation handler
// prints the function, file, line, comment and message, as it always has,
// for a precondition, one with a user message and a contract_assert;
// contract-violation-information-default-handler.C is the same under none.
//
// Mirror, in the llvm_llvm-project fork:
// clang/test/Contracts/Runnable/contract-violation-information-default-handler-full.cpp
// (libc++'s handler has its own format); in the edgcpp_compiler fork,
// tests/tests/contracts/vinfo/default_handler.sft.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-violation-information=full" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

void pre_f (int x) pre (x > 0) { }
void msg_f (int x) pre (x > 0, "a user message") { }
void assert_f (int x) { contract_assert (x > 0); }

int
main ()
{
  pre_f (0);
  msg_f (0);
  assert_f (0);
}

// { dg-output "^contract violation in function void pre_f.int. at \[^\n\r\]*contract-violation-information-default-handler-full.C:15: x > 0(\n|\r\n|\r)\\\[assertion_kind: pre, semantic: observe, mode: predicate_false, terminating: no\\\](\n|\r\n|\r)" }
// { dg-output "contract violation in function void msg_f.int. at \[^\n\r\]*contract-violation-information-default-handler-full.C:16: x > 0 .a user message.(\n|\r\n|\r)\\\[assertion_kind: pre, semantic: observe, mode: predicate_false, terminating: no\\\](\n|\r\n|\r)" }
// { dg-output "contract violation in function void assert_f.int. at \[^\n\r\]*contract-violation-information-default-handler-full.C:17: x > 0(\n|\r\n|\r)\\\[assertion_kind: assert, semantic: observe, mode: predicate_false, terminating: no\\\](\n|\r\n|\r)$" }
