// The result binding is const in a postcondition's predicate, including
// through a by-reference capture of a lambda there, and it can be named in a
// capture list.  (GCC rejects `[r] () mutable { return ++r; }', which is
// valid; that shape is left out.)
//
// Mirror: clang/test/Contracts/result-binding-lambda-ref-constify.cpp and
// result-binding-lambda-explicit-capture.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int b0 () post (r: [r] { return r; } () > 0) { return 1; }
int b2 () post (r: [&] { return ++r; } () > 0) { return 1; }	// { dg-error "read-only" }
