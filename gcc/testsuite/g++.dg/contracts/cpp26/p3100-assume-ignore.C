// P3100: selecting the "assume" semantic without -fcontracts-allow-assume
// resolves to "ignore" -- the predicate is not evaluated and no violation
// is reported, so a failing precondition is a no-op.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontract-evaluation-semantic=assume" }

int side_effect = 0;

bool check(int i) { ++side_effect; return i > 0; }

int f(int i) pre(check(i)) { return i; }

int main()
{
  f(-1);                     // would violate if enforced
  if (side_effect != 0)      // "ignore" must not evaluate the predicate
    __builtin_abort();
  return 0;
}
