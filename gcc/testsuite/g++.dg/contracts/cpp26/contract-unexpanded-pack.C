// A parameter pack named unexpanded in a contract predicate is diagnosed in
// the template, as in any other expression (GCC-651).  The predicate never
// went through check_for_bare_parameter_packs, so the template compiled
// silently and an instantiation reported "sorry, unimplemented: use of
// 'type_pack_expansion' in template", or nothing at all (stock GCC too).
//
// Mirror: clang/test/Contracts/contract-unexpanded-pack.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template <class... T>
int f1 (T... xs) pre (xs > 0) { return 0; }		// { dg-error "not expanded" }
template <class... T>
int f2 (T... xs) post (r: r > xs) { return 0; }		// { dg-error "not expanded" }
template <class... T>
int f3 (T... xs) { contract_assert (xs > 0); return 0; }	// { dg-error "not expanded" }
template <class... T>
int f4 (T... xs) pre (xs > 0);				// { dg-error "not expanded" }
template <class... T>
struct S { int m (T... xs) pre (xs > 0) { return 0; } };	// { dg-error "not expanded" }

// Expanded, they are fine.
template <class... T>
int g1 (const T... xs) pre (((xs > 0) && ...)) post (r: ((r > xs) || ...))
{ contract_assert (((xs > 0) && ...)); return 2; }
template <class... T>
int g2 (T... xs)
{ contract_assert ([=] { return ((xs > 0) && ...); } ()); return 0; }

int main () { return g1 (1, 1) + g2 (1, 2) - 2; }
