// Under P3595 caller-side checking (caller observe, callee ignore), the
// caller-side wrapper of a constexpr function template -- or of a static
// member function of a class template -- defined on demand from the constant
// evaluator substitutes the contracts itself, and the valid constant
// evaluations below are accepted.
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }

template <class T> constexpr T t (T x) pre (x > 0) { return x; }
static_assert (t (4) == 4);

template <class U> struct C { static constexpr U s (U x) pre (x > 0) { return x; } };
static_assert (C<int>::s (4) == 4);

constexpr int n (int x) pre (x > 0) { return x; }
static_assert (n (4) == 4);
