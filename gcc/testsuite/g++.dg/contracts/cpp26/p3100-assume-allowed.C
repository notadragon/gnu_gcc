// P3100: with -fcontracts-allow-assume the "assume" semantic survives
// resolution, but its code generation is identical to "ignore" for now --
// no check is emitted, the predicate is not evaluated, and a failing
// precondition does not report a violation.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-evaluation-semantic=assume -fcontracts-allow-assume" }

int side_effect = 0;

bool check(int i) { ++side_effect; return i > 0; }

int f(int i) pre(check(i)) { return i; }

int main()
{
  f(-1);                     // assume emits no check (same as ignore for now)
  if (side_effect != 0)
    __builtin_abort();
  return 0;
}
