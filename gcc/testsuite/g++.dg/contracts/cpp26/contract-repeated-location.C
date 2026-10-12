// A contract assertion repeated on a later declaration (the definition) and
// violated is reported, through contract_violation::location(), at the
// definition's assertion: for a non-template function, a member function
// defined outside its class, a member of a class template with a non-deduced
// and with a deduced return type, and a function template; preconditions
// and postconditions.  The location is implementation-defined and the
// assertions are equivalent; the EDG front end reports the first
// declaration's (except for a template through edg-gxx), and Clang the
// definition's except for a class template member with a non-deduced return
// type (a divergence, GCC-703 in open-issues/README.md).
// contract-repeated-location-constexpr.C has the same shapes in constant
// evaluation.
//
// Mirrors: clang/test/Contracts/contract-repeated-location.cpp in the
// llvm_llvm-project fork; in the edgcpp_compiler fork,
// tests/tests/contracts/codegen/repeated_contract_location.sft.cpp and the
// imports tests/tests/contracts/imported/{gcc,clang}/
// contract-repeated-location.sft.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static unsigned last;
static int count;

void
handle_contract_violation (const std::contracts::contract_violation &v)
{
  last = v.location ().line ();
  ++count;
}

// Expect exactly one violation since the last check, at the definition.
static void
check (unsigned first, unsigned definition)
{
  if (count != 1 || last == first || last != definition)
    __builtin_abort ();
  last = 0;
  count = 0;
}

int f (const int x) pre (x > 0); enum { FPRE1 = __LINE__ };
int f (const int x) pre (x > 0) { return x; } enum { FPRE2 = __LINE__ };
int fp (const int x) post (r : r > x); enum { FPOST1 = __LINE__ };
int fp (const int x) post (r : r > x) { return x; } enum { FPOST2 = __LINE__ };

struct S
{
  int m (const int x) const pre (x > 0); enum { MPRE1 = __LINE__ };
  int mp (const int x) const post (r : r > x); enum { MPOST1 = __LINE__ };
};
int S::m (const int x) const pre (x > 0) { return x; } enum { MPRE2 = __LINE__ };
int S::mp (const int x) const post (r : r > x) { return x; } enum { MPOST2 = __LINE__ };

template <class T> struct Z
{
  T g (const T x) const pre (x > 0); enum { GPRE1 = __LINE__ };
  T gp (const T x) const post (r : r > x); enum { GPOST1 = __LINE__ };
  auto a (const T x) const pre (x > 0); enum { APRE1 = __LINE__ };
  auto ap (const T x) const post (r : r > x); enum { APOST1 = __LINE__ };
};
template <class T>
T Z<T>::g (const T x) const pre (x > 0) { return x; } enum { GPRE2 = __LINE__ };
template <class T>
T Z<T>::gp (const T x) const post (r : r > x) { return x; } enum { GPOST2 = __LINE__ };
template <class T>
auto Z<T>::a (const T x) const pre (x > 0) { return x; } enum { APRE2 = __LINE__ };
template <class T>
auto Z<T>::ap (const T x) const post (r : r > x) { return x; } enum { APOST2 = __LINE__ };

template <class T> T h (const T x) pre (x > 0); enum { HPRE1 = __LINE__ };
template <class T> T h (const T x) pre (x > 0) { return x; } enum { HPRE2 = __LINE__ };
template <class T> T hp (const T x) post (r : r > x); enum { HPOST1 = __LINE__ };
template <class T> T hp (const T x) post (r : r > x) { return x; } enum { HPOST2 = __LINE__ };

int
main ()
{
  f (0);
  check (FPRE1, FPRE2);
  fp (0);
  check (FPOST1, FPOST2);

  S{}.m (0);
  check (S::MPRE1, MPRE2);
  S{}.mp (0);
  check (S::MPOST1, MPOST2);

  Z<int>{}.g (0);
  check (Z<int>::GPRE1, GPRE2);
  Z<int>{}.gp (0);
  check (Z<int>::GPOST1, GPOST2);
  Z<int>{}.a (0);
  check (Z<int>::APRE1, APRE2);
  Z<int>{}.ap (0);
  check (Z<int>::APOST1, APOST2);

  h (0);
  check (HPRE1, HPRE2);
  hp (0);
  check (HPOST1, HPOST2);
}
