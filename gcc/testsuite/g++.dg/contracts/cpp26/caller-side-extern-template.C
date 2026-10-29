// Under P3595 caller-side checking, a call to a function-template
// specialization named in an explicit instantiation declaration is accepted:
// the caller-side wrapper substitutes the contracts itself, since the
// definition is not instantiated in this translation unit.
//
// No Clang mirror: Clang has no caller-side checking.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/open-bug-caller-side.json" }

template <class T> T g (T x) pre (x > 0) { return x; }
extern template int g<int> (int);

int main () { return g (1); }
