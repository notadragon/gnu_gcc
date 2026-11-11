// A contract check lets the code after it rely on its predicate, but a
// function another translation unit may also define (here an inline
// function) has a refinable body when it holds a contract assertion: the
// other definition may evaluate the assertion differently, for example
// ignore it, so callers that do not inline the function must not take its
// return value range from this body.  The static and the external
// non-inline function have no other definition, so their callers fold the
// comparison; inside the inline function the check still folds the
// comparison after it.  No noipa wrapper is involved.
//
// Mirror: clang/test/Contracts/refinable-body-enforce.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce -O2 -fno-ipa-sra -fdump-tree-optimized" }

[[gnu::noinline]] inline int fi (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] static int fs (int x) { contract_assert (x > 0); return x; }
[[gnu::noinline]] int fe (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] inline int hi (int x)
{
  contract_assert (x > 0);
  return x > 0 ? 1234 : 5678;
}

int use_i (int y) { return fi (y) > 0; }
int use_s (int y) { return fs (y) > 0; }
int use_e (int y) { return fe (y) > 0; }
int use_h (int y) { return hi (y); }

// { dg-final { scan-tree-dump-times "= _\[0-9\]+ > 0;" 1 "optimized" } }
// { dg-final { scan-tree-dump "return 1234;" "optimized" } }
// { dg-final { scan-tree-dump-not "5678" "optimized" } }
// { dg-final { scan-assembler-not "__tu_terminate_wrapper" } }
