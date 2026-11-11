// P4405: a static operator() is called with its object as the first
// argument of the call, evaluated and ignored, so the arguments the
// precondition's parameters are given at a call start after it, for
// folding (-O0) and for the proof shadow (-O2) alike: with pre (b > 0),
// s (1, -1) is always false and s (-1, 1) is not diagnosed.  Also a static
// lambda, whose predicate names no capture, and a lambda: their
// preconditions are proven at a call from the arguments.  The same as p4405-fold-static-operator.C, at -O2.
//
// Mirror: clang/test/Contracts/p4405-fold-static-operator.cpp in the
// llvm_llvm-project fork (CLANG-854: Clang's folding gave each parameter the
// argument before its own).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O2" }

struct S {
  static int operator() (int a, int b) pre (b > 0);	// { dg-message "the predicate of the contract assertion" }
};

int
use (S s)
{
  int r = s (-1, 1);	// { dg-bogus "(always false|cannot be proven)" }
  auto l = [] (int a, int b) static pre (b > 0) { return a + b; };
  r += l (-1, 1);	// { dg-bogus "(always false|cannot be proven)" }
  auto m = [] (int a) pre (a > 0) { return a; };
  r += m (1);		// { dg-bogus "(always false|cannot be proven)" }
  return r;
}

/* Each always false evaluation in a function of its own: in the shadow,
   what follows one is never reached (DECISIONS.md E196 item 1).  */

int
use_s (S s)
{
  return s (1, -1);	// { dg-error "contract predicate is always false" }
}

int
use_l ()
{
  auto l = [] (int a, int b) static pre (b > 0) { return a + b; };	// { dg-message "the predicate of the contract assertion" }
  return l (1, -1);	// { dg-error "contract predicate is always false" }
}
