// A non-reference parameter odr-used in a postcondition must be const on every
// declaration of the function ([dcl.contract.func]), including declarations
// without the postcondition: a non-const non-defining redeclaration and a
// non-const out-of-line class-template member definition are rejected.
//
// Mirrors: clang/test/Contracts/redecl-post-param-const.cpp and
// redecl-post-param-const-class-template.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void f (const int x) post (x > 0);
void f (int x);			// { dg-error "must be const" }
void f (const int x) {}

template <class T> struct S { void m (const T x) post (x > 0); };
template <class T> void S<T>::m (T x) {}	// { dg-error "must be const" }
void u () { S<int>{}.m (1); }
