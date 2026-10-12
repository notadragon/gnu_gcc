// Code generation with debug information for the contract checks whose
// predicate or P3098 capture may throw, which are wrapped in a handler that
// has no statement of its own in the source: pre, post and contract_assert
// in a free function, a capture with a throwing copy, a lambda and a
// function template.  It must compile.
//
// Mirror: clang/test/Contracts/codegen-debug-info.cpp (-DTHROWS) in the
// llvm_llvm-project fork (CLANG-129; GCC-668).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -g" }

bool may_throw (int);
struct C { C (); C (const C &); };

void pred (const int x) pre (may_throw (x)) post (may_throw (x))
{
  contract_assert (may_throw (x));
}
void capture (C c) post [d = c] (true) {}
auto lambda = [] (int x) pre (may_throw (x)) { return [x] { return x; } (); };
template <class T> void tmpl (const T x) pre (may_throw (x)) post [y = x] (y == x) {}
template void tmpl<int> (int);

int use () { return lambda (1); }
