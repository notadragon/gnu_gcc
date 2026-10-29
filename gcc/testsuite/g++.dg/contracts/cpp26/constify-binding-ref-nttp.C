// A structured binding whose variable is declared outside a contract
// predicate, of any form, and a template parameter of reference type name
// const lvalues in the predicate ([expr.prim.id.unqual]).
//
// Mirror: clang/test/Contracts/constify-structured-binding.cpp and
// clang/test/Contracts/constify-ref-nttp.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct P { int x, y; };

void b1 () { auto [a, b] = P{}; contract_assert (++a > 0); }	// { dg-error "read-only" }
void b2 () { int arr[1] = {0}; auto &[a] = arr; contract_assert (++a > 0); } // { dg-error "read-only" }
void b3 (P p) { auto &[a, b] = p; contract_assert ((b = 1) > 0); } // { dg-error "read-only" }

template <int &R> void t1 () pre (++R > 0) {}			// { dg-error "read-only" }
template <int &R> void t2 () { contract_assert (++R > 0); }	// { dg-error "read-only" }

int g;
void use () { t1<g> (); t2<g> (); }

void c1 (int x) { int &r = x; contract_assert (++r > 0); }	// { dg-error "read-only" }
