// When a postcondition's violation handler throws, the already initialized
// returned object is destroyed ([except.ctor]/2): for a plain postcondition,
// one with a capture, and the static type's postcondition in a P3097 wrapper.
//
// Mirror: clang/test/Contracts/retval-leak-throwing-post-capture.cpp and
// retval-leak-throwing-post-p3097-wrapper.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int ctor = 0, dtor = 0;
struct R {
  R () { ++ctor; }
  R (const R&) { ++ctor; }
  ~R () { ++dtor; }
};

void handle_contract_violation (const std::contracts::contract_violation&)
{
  throw 7;
}

// P3098 capture.
R f (int x) post [c = x] (c < 0) { return R (); }

// P3097: only the interface (Base) postcondition fails.
struct Base { virtual R f (const int x) post (x < 0); };
R Base::f (const int) { return R (); }
struct Der : Base { R f (const int x) override post (x > 0); };
R Der::f (const int) { return R (); }
Der d;
Base& rb = d;

int main ()
{
  try { R r = f (1); } catch (int) {}
  if (ctor != 1 || dtor != 1)
    __builtin_abort ();
  try { R r = rb.f (1); } catch (int) {}
  if (ctor != 2 || dtor != 2)
    __builtin_abort ();
}
