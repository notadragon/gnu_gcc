// A parameter a postcondition odr-uses must be const in every declaration,
// but a reference parameter is exempt ([dcl.contract.func]).  For a
// function template redeclared without the postcondition, each instance is
// checked: f<int> is rejected, and g<int &>, whose parameter is a reference,
// is accepted (GCC-642).
//
// Mirror: clang/test/Contracts/postcondition-params-template-redecl.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

template <typename T> void f (T const a) post (a);
template <typename T> void f (T a);
template <typename T> void f (T const a) {}  // { dg-error "must be const" }
template void f<int> (int);

template <typename T> void g (T const a) post (a);
template <typename T> void g (T a);
template <typename T> void g (T const a) {}  // { dg-bogus "must be const" }
template void g<int &> (int &);

template <typename T> void h (T const a) post (a);
template <typename T> void h (T a);
template <typename T> void h (T const a) {}  // OK, const in h<const int>
template void h<const int> (const int);
