// GCC-682: an array element an initializer list does not initialize
// explicitly is initialized by the list's array filler, so a
// potentially-throwing default constructor there makes the list
// potentially-throwing ([except.spec]/6).  The VEC_INIT_EXPR standing for
// the filler did not carry the constructor call, and noexcept was true.
//
// Mirror: clang/test/SemaCXX/noexcept-array-filler.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++20 } }

struct S { S () { throw 2; } };
struct N { N () noexcept; };
struct A { S s[2]; };
struct B { N n[2]; };
struct C { int i; S s[2]; };

static_assert (!noexcept (A{}));
static_assert (!noexcept (A{{}}));
static_assert (!noexcept (C{1}));
using S2 = S[2];
static_assert (!noexcept (S2{}));

static_assert (noexcept (B{}));
using N2 = N[2];
static_assert (noexcept (N2{}));
using I4 = int[4];
static_assert (noexcept (I4{1}));

template <class T> constexpr bool fill () { return noexcept (T{}); }
static_assert (!fill<A> ());
static_assert (fill<B> ());
