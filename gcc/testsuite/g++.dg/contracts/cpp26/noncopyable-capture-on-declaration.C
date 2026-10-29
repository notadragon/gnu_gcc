// A postcondition capture must be copy-initializable from its initializer,
// and a capture that is not is diagnosed on any declaration, at the capture,
// and only once for a function that is also defined.
//
// Mirror: clang/test/Contracts/noncopyable-capture-on-declaration.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }
// { dg-prune-output "declared here" }

struct NC { NC (int) {} NC (const NC &) = delete; };

int f (NC p) post [p] (true);      // { dg-error "deleted" }
int g (NC p) post [q = p] (true);  // { dg-error "deleted" }

int h (NC p) post [p] (true)       // { dg-error "deleted" }
{
  return 0;
}                                  // { dg-bogus "deleted" }

auto l = [] (NC p) post [p] (true) { return 0; };  // { dg-error "deleted" }

// Copyable captures, and a capture in a template whose type is dependent,
// are unaffected.
struct C { C (int) {} };
int ok (C p) post [p] (true);
template <class T> int t (T p) post [p] (true);
