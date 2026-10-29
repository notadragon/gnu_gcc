// A contract assertion repeated on a later declaration (the definition) and
// violated in constant evaluation is diagnosed at the definition's
// assertion, never the first declaration's: the shapes of
// contract-repeated-location.C (a non-template function, a member function
// defined outside its class, a member of a class template with a
// non-deduced and with a deduced return type, and a function template;
// preconditions and postconditions).  The location is implementation-defined
// and the assertions are equivalent; the EDG front end reports the first
// declaration's, and Clang the definition's except for a class template
// member with a non-deduced return type (a divergence, GCC-703 in
// open-issues/README.md).
//
// Mirrors: clang/test/Contracts/contract-repeated-location-constexpr.cpp in
// the llvm_llvm-project fork; in the edgcpp_compiler fork,
// tests/tests/contracts/codegen/repeated_contract_location_constexpr.sft.cpp
// and the imports tests/tests/contracts/imported/{gcc,clang}/
// contract-repeated-location-constexpr.sft.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

constexpr int f (const int x) pre (x > 0);
constexpr int f (const int x) pre (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int fp (const int x) post (r : r > x);
constexpr int fp (const int x) post (r : r > x) { return x; }	// { dg-error "contract predicate is false in constant expression" }

struct S
{
  constexpr int m (const int x) const pre (x > 0);
  constexpr int mp (const int x) const post (r : r > x);
};
constexpr int S::m (const int x) const pre (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
constexpr int S::mp (const int x) const post (r : r > x) { return x; }	// { dg-error "contract predicate is false in constant expression" }

template <class T> struct Z
{
  constexpr T g (const T x) const pre (x > 0);
  constexpr T gp (const T x) const post (r : r > x);
  constexpr auto a (const T x) const pre (x > 0);
  constexpr auto ap (const T x) const post (r : r > x);
};
template <class T>
constexpr T Z<T>::g (const T x) const pre (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
template <class T>
constexpr T Z<T>::gp (const T x) const post (r : r > x) { return x; }	// { dg-error "contract predicate is false in constant expression" }
template <class T>
constexpr auto Z<T>::a (const T x) const pre (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
template <class T>
constexpr auto Z<T>::ap (const T x) const post (r : r > x) { return x; }	// { dg-error "contract predicate is false in constant expression" }

template <class T> constexpr T h (const T x) pre (x > 0);
template <class T> constexpr T h (const T x) pre (x > 0) { return x; }	// { dg-error "contract predicate is false in constant expression" }
template <class T> constexpr T hp (const T x) post (r : r > x);
template <class T> constexpr T hp (const T x) post (r : r > x) { return x; }	// { dg-error "contract predicate is false in constant expression" }

// The holding calls are constant (the postconditions here never hold).
static_assert (f (1) == 1);	// OK
static_assert (S{}.m (1) == 1);	// OK
static_assert (Z<int>{}.g (1) == 1);	// OK
static_assert (Z<int>{}.a (1) == 1);	// OK
static_assert (h (1) == 1);	// OK

constexpr int bad_f = f (0);	// Error, at f's definition
constexpr int bad_fp = fp (0);	// Error, at fp's definition
constexpr int bad_m = S{}.m (0);	// Error, at S::m's definition
constexpr int bad_mp = S{}.mp (0);	// Error, at S::mp's definition
constexpr int bad_g = Z<int>{}.g (0);	// Error, at Z::g's definition
constexpr int bad_gp = Z<int>{}.gp (0);	// Error, at Z::gp's definition
constexpr int bad_a = Z<int>{}.a (0);	// Error, at Z::a's definition
constexpr int bad_ap = Z<int>{}.ap (0);	// Error, at Z::ap's definition
constexpr int bad_h = h (0);	// Error, at h's definition
constexpr int bad_hp = hp (0);	// Error, at hp's definition
