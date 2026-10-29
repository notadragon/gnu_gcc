// P3100: a precondition evaluated caller-side with the assume semantic is an
// optimizer assumption at the call: the wrapper making it is inlined into
// the caller, which may then take the predicate to hold after the call, so
// the test below it is removed.  The predicate is never evaluated (here it
// counts its evaluations).  The callee's own evaluation is ignore.
//
// Clang: CLANG-111 in the llvm_llvm-project fork (its assume semantic
// lowers to ignore).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-allow-assume -O2 -fdump-tree-optimized" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-semantic-assume-caller.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int evaluations = 0;
int dead_calls = 0;

[[gnu::noinline]] bool positive (int i) { ++evaluations; return i > 0; }
[[gnu::noinline]] void dead () { ++dead_calls; }

[[gnu::noinline]] void take (int x)
  pre (x > 0);

[[gnu::noinline]] void count (int x)
  pre (positive (x));

[[gnu::noinline]] int use (int x)
{
  take (x);
  if (x <= 0)
    dead ();
  count (x);
  return x;
}

void take (int x) pre (x > 0) { }
void count (int x) pre (positive (x)) { }

int main ()
{
  if (use (3) != 3 || evaluations != 0 || dead_calls != 0)
    __builtin_abort ();
  return 0;
}

// { dg-final { scan-tree-dump-not "dead \\(\\);" "optimized" } }
