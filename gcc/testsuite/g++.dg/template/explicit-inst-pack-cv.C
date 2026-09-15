// Explicit instantiation of a variadic function template fails to match when a
// template argument bound to the function parameter pack is cv-qualified.  The
// instantiations below are well-formed and should compile, but GCC rejects them
// with "template-id ... does not match any template declaration".  They are
// marked xfail (dg-bogus) so this file records the bug and will XPASS once it is
// fixed.  Clang accepts all of these.
//
// This is NOT a contracts bug; it was found while testing C++26 contracts but
// reproduces in plain C++11 with no contract and no pack indexing, and on GCC
// 13.4 as well as trunk.  Upstream PR126797.
//
// Root cause, fix options and their risks: bug-reports/gcc-39-explicit-inst-pack-cv.md
// in this fork.  They are a design discussion, not something a reader of the
// testsuite needs in front of them.

// { dg-do compile { target c++11 } }

template <typename... Ts> void f (Ts...) {}

// The bug: well-formed explicit instantiations that GCC currently rejects.
template void f<const int> (const int);
// { dg-bogus "does not match any template declaration" "cv-qualified pack arg (PR126797)" { xfail *-*-* } .-1 }

template void f<int * const> (int *);
// { dg-bogus "does not match any template declaration" "cv-qualified pack arg (PR126797)" { xfail *-*-* } .-1 }

// Controls that already behave correctly (documenting the scope of the bug).

// Non-variadic template with a cv-qualified argument: matches (the by-value
// parameter adjustment is applied on the complete-substitution path).
template <typename T> void g (T) {}
template void g<const int> (const int);   // OK

// Variadic template without cv-qualification on the pack argument: matches.
template void f<int> (int);               // OK
