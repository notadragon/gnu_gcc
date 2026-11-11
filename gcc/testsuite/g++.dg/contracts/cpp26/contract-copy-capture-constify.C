// In a contract_assert inside a mutable lambda, a name referring to a by-copy
// capture is not modifiable: the capture is a member of the closure object
// and is constified, so each line below is ill-formed.  The by-reference
// sibling is contract-ref-capture-constify.C.
//
// Mirror: clang/test/Contracts/contract-copy-capture-constify.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void a (int x) { [=]() mutable { contract_assert (++x > 0); (void) x; }(); } // { dg-error "read-only" }
void b (int x) { [x]() mutable { contract_assert (++x > 0); }(); }           // { dg-error "read-only" }
void c (int x) { [y = x]() mutable { contract_assert (++y > 0); }(); }       // { dg-error "read-only" }
void d () { int x = 0; [x]() mutable { contract_assert (++x > 0); }(); }     // { dg-error "read-only" }
template <class T>
void e (T x) { [x]() mutable { contract_assert (++x > 0); }(); }             // { dg-error "read-only" }
template void e (int);
