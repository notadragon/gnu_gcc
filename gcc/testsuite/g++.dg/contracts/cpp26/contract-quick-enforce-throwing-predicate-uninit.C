// A potentially-throwing predicate checked under quick_enforce must not draw
// a -Wmaybe-uninitialized (or, once inlined, -Wuninitialized) warning about a
// compiler temporary, e.g. "'D.1234' may be used uninitialized".
//
// The check stores the predicate's result in a temporary inside a try block
// and reads it afterwards.  The quick_enforce reaction in the catch (...)
// handler does not return, so it never reaches the read.
// Stock GCC warns (GCC-689 in this repository's bug-reports/, PR125163
// comment 2): its reaction is a call to a noipa wrapper that is not
// noreturn, so the temporary appears to reach the read unset.
//
// Mirror: clang/test/Contracts/contract-quick-enforce-throwing-predicate-uninit.cpp
// in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=quick_enforce -O2 -Wall -Wextra" }

bool positive (int);

void pre_fn (int x) pre (positive (x)) { }	// { dg-bogus "uninitialized" }

int post_fn (int x) post (r: positive (r)) { return x; }	// { dg-bogus "uninitialized" }

void assert_fn (int x) { contract_assert (positive (x)); }	// { dg-bogus "uninitialized" }

int
main ()
{
  pre_fn (1);
  assert_fn (2);
  return post_fn (3);
}
