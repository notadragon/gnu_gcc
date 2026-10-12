// Under the assume semantic a contract predicate is not evaluated during
// constant evaluation: a false or non-constant predicate neither makes the
// evaluation non-constant nor turns into a silent substitution failure in a
// concept.
//
// Mirror: clang/test/Contracts/ce-assume-nonconstant-predicate.cpp and
// ce-assume-false-predicate.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-allow-assume -fcontract-evaluation-semantic=assume" }

constexpr int f (int x) pre (x > 0) { return x; }
int g (int x) { return x; }
constexpr int h (int x) pre (g (x) > 0) { return x; }

constexpr int a = f (-1);          // false predicate
constexpr int b = h (1);           // non-constant predicate

template <int N> struct K {};
template <int V> concept CF = requires { typename K<f (V)>; };
template <int V> concept CH = requires { typename K<h (V)>; };
static_assert (CF<-1>);
static_assert (CH<1>);
