// P3099: the user-defined message appears in the compile-time diagnostic for
// a violation during constant evaluation -- as the warning observe gives,
// the call remaining a constant -- and only where a violation actually
// occurs: not in an unevaluated operand, not in a discarded statement, and
// not in the trial evaluation that decides whether a variable is
// constant-initialized, which treats every contract assertion as ignored
// ([expr.const]; P3099R3 sec. "Constant evaluation").  Each context has its
// own function, so a stray diagnostic lands on that function's line.
//
// Mirror: clang/test/Contracts/p3099-message-observe-constexpr.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }

int rt;

constexpr int
f (int x) pre (x > 0, "need positive") // { dg-warning "contract predicate is false in constant expression \\(need positive\\)" }
{
  return x;
}
constexpr int observed = f (-1);
static_assert (observed == -1);
// Constant initialization, so manifestly constant-evaluated: also warned.
const int static_init = f (-2);

// Unevaluated operands.
constexpr int u (int x) pre (x > 0, "unevaluated") { return x; } // { dg-bogus "unevaluated" }
static_assert (sizeof (u (-1)) == sizeof (int));
using T = decltype (u (-1));

// A discarded statement in a template.
constexpr int d (int x) pre (x > 0, "discarded") { return x; } // { dg-bogus "discarded" }
template <bool B>
int
g ()
{
  if constexpr (B)
    {
      constexpr int never = d (B - 5);
      return never;
    }
  return 0;
}
int gi = g<false> ();

// Trial evaluation: with its assertions ignored, t (0) reads a non-constant
// variable, so the variable is dynamically initialized and its initializer
// is not manifestly constant-evaluated (the [expr.const] example's i0).
constexpr int t (int c) pre (c > 0, "trial") { return c == 0 ? rt : c; } // { dg-bogus "trial" }
const int dynamic_init = t (0);
