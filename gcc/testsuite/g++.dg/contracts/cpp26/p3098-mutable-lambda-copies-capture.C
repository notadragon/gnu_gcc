// A postcondition capture (P3098) is declared inside the contract assertion
// and is not constified (D3098R3), so a mutable lambda in the predicate that
// captures it by copy has a member of its type (int) and may modify that
// copy, also in a nested mutable lambda.  The capture of a const parameter
// is an int too (deduced as `auto' would be).  A non-mutable lambda's copy
// is const, as anywhere ([expr.prim.lambda.capture]), also where it copies
// a mutable lambda's member (GCC-662).
//
// Mirror: clang/test/Contracts/p3098-mutable-lambda-copies-capture.cpp in
// the llvm_llvm-project fork (CLANG-643).  The run-time half is
// p3098-lambda-copies-capture.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }

int k (int y) post [old = y] ([=] () mutable { return ++old > 1; } ()) { return y; }	// OK
int k2 (const int y) post [old = y] ([=] () mutable { return ++old > 1; } () && old == y)
{ return y; }	// OK
int k3 (int y)
  post [old = y] ([=] () mutable { return [=] () mutable { return ++old; } () > 1; } ())
{ return y; }	// OK
int k4 (int y) post [old = y] ([=] { return ++old > 1; } ()) { return y; }	// { dg-error "read-only" }
int k5 (int y)
  post [old = y] ([=] () mutable { return [=] { return ++old; } () > 1; } ())	// { dg-error "read-only" }
{ return y; }
