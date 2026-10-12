// A potentially-throwing predicate checked under quick_enforce must not draw
// a -Wmaybe-uninitialized (or, once inlined, -Wuninitialized) warning about a
// compiler temporary, e.g. "'D.1234' may be used uninitialized".
//
// The check stores the predicate's result in a temporary inside a try block
// and reads it afterwards.  The catch (...) handler reacts with a call to the
// noipa quick_enforce wrapper, which is not noreturn, so the handler must
// also set the temporary or it appears to reach the read unset.  Stock GCC
// warns (GCC-689 in this repository's bug-reports/, PR125163 comment 2).
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
