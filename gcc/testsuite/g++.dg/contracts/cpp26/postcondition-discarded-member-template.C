// A function template's by-value parameter named only as the object of a
// discarded member access naming a non-static data member in a
// postcondition is not odr-used ([basic.def.odr]/3, /5), so
// [dcl.contract.func]'s const requirement does not reach it, and the
// definition may drop the const (GCC-663: the dependent access was taken as
// an odr-use before substitution).  The non-template form is
// postcondition-discarded-member-access.C (GCC-650).
//
// An access naming a static member or an enumerator does odr-use its
// object, which is not among its potential results; the object is dropped
// from the tree when it has no side effects, and that was accepted (GCC-666).
//
// Mirror: clang/test/Contracts/postcondition-discarded-member-access.cpp in
// the llvm_llvm-project fork (its `re_tmpl'); the static-member cases are
// clang/test/Contracts/postcondition-discarded-member-template.cpp there.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce -Wno-unused-value" }

struct S { int m; };
struct T { static int sm; static bool sf (); enum { E = 1 }; int m; };

template <class U> void re_tmpl (const U s) post ((s.m, true));
template <class U> void re_tmpl (U s) {}	// { dg-bogus "postcondition must be const" }
template void re_tmpl<S> (S);

template <class U> void mem (U s) post ((s.m, true)) {}	// { dg-bogus "postcondition must be const" }
template void mem<S> (S);

// The static member through the dependent object.
template <class U> void st (U s) post ((s.sm, true)) {}	// { dg-error "postcondition must be const" }
template void st<T> (T);
template <class U> void st_re (U s) post ((s.sm, true));	// { dg-message "declared .T. here, which is not const" }
template <class U> void st_re (const U s) {}	// { dg-error "postcondition must be const" }
template void st_re<T> (T);

// And without a template (GCC-666).
void ns_sm (T s) post ((s.sm, true)) {}	// { dg-error "postcondition must be const" }
void ns_sm2 (T s) post (s.sm > 0) {}	// { dg-error "postcondition must be const" }
void ns_sf (T s) post (s.sf ()) {}	// { dg-error "postcondition must be const" }
void ns_e (T s) post (s.E > 0) {}	// { dg-error "postcondition must be const" }
void ns_sizeof (T s) post (sizeof (s.sm) > 0) {}	// OK, unevaluated
void ns_const (const T s) post (s.sm > 0 && s.sf () && s.E > 0) {}	// OK
void ns_lambda (T t) post ([] (T s) { return s.sm > 0; } (T{})) {}	// OK, the lambda's own
