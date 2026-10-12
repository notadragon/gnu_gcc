// A parameter named in a precondition's predicate, or in a P3098 capture's
// initializer, is used there, so assigning it in the body does not make it
// "set but not used", on a function definition and on a member function
// defined in its class.  A parameter its contracts do not name still warns
// (GCC-662).
//
// Mirror: clang/test/Contracts/param-set-used-in-contract.cpp in the
// llvm_llvm-project fork (CLANG-633: Clang warned).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -Wall -Wextra" }

int f (int i) pre (i > 0) { i = 2; return 0; }  // { dg-bogus "set but not used" }
int g (int i) post [j = i] (r: r != j) { i = 2; return 0; }  // { dg-bogus "set but not used" }
int h (int i) { i = 2; return 0; }  // { dg-warning "set but not used" }
struct S
{
  int m (int i) pre (i > 0) { i = 2; return 0; }  // { dg-bogus "set but not used" }
  int m2 (int i) post [j = i] (r: r != j) { i = 2; return 0; }  // { dg-bogus "set but not used" }
  int n (int i, int k) pre (k > 0) { i = 2; return k; }  // { dg-warning "parameter 'i' set but not used" }
};
int n (int i, int k) pre (k > 0) { i = 2; return k; }  // { dg-warning "parameter 'i' set but not used" }
