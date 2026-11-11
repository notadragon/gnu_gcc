// After a malformed postcondition capture such as `post[1]', only syntax
// errors are reported: the predicate is never parsed outside the parameter
// scope, so there is no complaint about `x'.
//
// Mirror: clang/test/Contracts/postcondition-capture-error-recovery.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

int k (int x) post[1] (x > 0); // { dg-error "expected identifier in postcondition capture" }
// { dg-bogus "not declared" "" { target *-*-* } .-1 }
// { dg-prune-output "before" }
