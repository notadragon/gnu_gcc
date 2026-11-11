// An explicit specialization of a member template of a class template
// specialization,
//
//   template<> template<class Q> int A<long>::f (long a, Q);
//
// has contracts of its own, not the member template's (GCC-627):
//  * a non-defining declaration's predicate finds the parameters by the
//    names it gives them;
//  * a later redeclaration that changes or adds contracts is diagnosed;
//  * a definition without contracts keeps the declaration's.
// Function templates and ordinary members: explicit-spec-decl-renamed-params.C,
// explicit-spec-contract-mismatch.C.
//
// Mirror: clang/test/Contracts/explicit-spec-member-template-contracts.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template<typename T>
struct A
{
  template<typename P> int f (T t, P) pre (t > 0) { return 0; }
};

// Renamed parameter on a non-defining declaration.
template<>
template<typename Q>
int A<long>::f (long a, Q) pre (a > 3);  // { dg-bogus "not declared" }
template<>
template<typename Q>
int A<long>::f (long a, Q) pre (a > 3) { return 2; }

// A redeclaration whose contract differs.
template<>
template<typename Q>
int A<char>::f (char t, Q) pre (t > 4);
template<>
template<typename Q>
int A<char>::f (char t, Q) pre (t > 5) { return 4; }  // { dg-error "mismatched contract" }

// A redeclaration that adds contracts.
template<>
template<typename Q>
int A<unsigned>::f (unsigned t, Q);
template<>
template<typename Q>
int A<unsigned>::f (unsigned t, Q) pre (t > 6) { return 5; }  // { dg-error "declaration adds contracts" }

// Controls: its own contracts, different from the primary's, are accepted
// on a definition, as is a declaration without contracts.
template<>
template<typename Q>
int A<double>::f (double t, Q) pre (t > 2) { return 1; }
template<>
template<typename Q>
int A<short>::f (short t, Q);
template<>
template<typename Q>
int A<short>::f (short t, Q) { return 3; }

// A definition without contracts keeps the declaration's, in terms of its
// own parameters.
template<typename T>
struct B
{
  template<typename P> constexpr int g (T t, P) pre (t > 0) { return 0; }
};
template<>
template<typename Q>
constexpr int B<int>::g (int a, Q) pre (a > 3);  // { dg-error "contract predicate is false" }
template<>
template<typename Q>
constexpr int B<int>::g (int b, Q) { return b; }
static_assert (B<int>{}.g (4, 0) == 4);
constexpr int bad = B<int>{}.g (2, 0);
