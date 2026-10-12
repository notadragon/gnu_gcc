// A name may appear only once in a P3098 capture list, whether it is
// introduced by an init-capture, a simple capture or an init-capture pack.
//
// Mirror: clang/test/Contracts/p3098-duplicate-capture.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }

int p7 (int x) post [y = x, y = x] (true) { return 0; }  // { dg-error "conflicts with a previous declaration|redeclaration" }
int p8 (int x) post [x, x] (true) { return 0; }  // { dg-error "conflicts with a previous declaration|redeclaration" }

template <class... T>
int p6 (T... xs) post [...y = xs, ...y = xs] (true) { return 0; }  // { dg-error "conflicts with a previous declaration|redeclaration" }

int main () { return p6 (1, 2); }
