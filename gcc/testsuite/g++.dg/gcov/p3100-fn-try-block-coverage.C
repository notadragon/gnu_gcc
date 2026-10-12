// Coverage of a value-returning function whose body is a function-try-block
// is the same with -fcontracts-p3100 as without.  (It lives here rather than in
// g++.dg/contracts because run-gcov needs gcov.exp.)
//
// Mirror: clang/test/Contracts/p3100-fn-try-block-profile.cpp and
// p3100-fn-try-block-mcdc.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-options "-fprofile-arcs -ftest-coverage -fcontracts -fcontracts-p3100" }

int g (int a, int b) try	/* count(1) */
{
  if (a && b)			/* count(1) */
    return 1;			/* count(#####) */
  return 2;			/* count(1) */
}
catch (...)
{
  return 3;
}

int main ()
{
  return g (1, 0) == 2 ? 0 : 1;	/* count(1) */
}

// { dg-final { run-gcov p3100-fn-try-block-coverage.C } }
