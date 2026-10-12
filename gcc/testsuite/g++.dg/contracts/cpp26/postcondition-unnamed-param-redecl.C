// A redeclaration that names no parameter a postcondition on another
// declaration odr-uses must still declare it const ([dcl.contract.func]);
// the error is given on the unnamed parameter of the redeclaration.  So
// must the definition of a function template, when it is instantiated
// (GCC-662).
//
// Mirror: clang/test/Contracts/postcondition-unnamed-param-diagnostic.cpp in
// the llvm_llvm-project fork (CLANG-640: its diagnostic printed a null
// name).  Found by the EDG fork's sema/postcondition_params_redecl test.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

void r (const int b) post (b > 0);
void r (int);	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }
void s (const int b) post (b > 0);
void s (const int);	// OK

// GCC reports the template twice, on the definition's parameter and on the
// declaration's postcondition: the two mechanisms implementing the rule
// anchor at different places on purpose (bug-reports/gcc-36, "the
// two-diagnostic template case").
template <class T> void t (const T b) post (b > 0);	// { dg-error "value parameter used in a postcondition must be const" }
template <class T> void t (T) {}	// { dg-error "value parameter (.<anonymous>.|.b.) used in a postcondition must be const" }
template void t<int> (int);
template <class T> void u (const T b) post (b > 0);
template <class T> void u (const T) {}	// OK
template void u<int> (int);
