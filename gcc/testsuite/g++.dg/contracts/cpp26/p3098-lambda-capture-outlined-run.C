// P3098: as p3098-lambda-capture-run.C, but with the checks outlined.  The
// outlined contract functions are declared when the lambda body starts, so
// the lambda's contracts must be parsed there, after its explicit captures
// are in scope; the capture proxy is remapped into the outlined function.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

int main ()
{
  int k = 5;
  auto deduced = [] (int x) post [c = x] (c == 3) { return x; };
  auto explicit_ret = [] (int x) -> int post [c = x] (c == 3) { return x; };
  auto with_capture = [k] (int x) post [c = x] (c == k) { return x + 1; };

  if (deduced (3) != 3 || violations != 0)
    __builtin_abort ();
  deduced (1);
  if (violations != 1)
    __builtin_abort ();
  explicit_ret (3);
  explicit_ret (1);
  if (violations != 2)
    __builtin_abort ();
  with_capture (5);
  with_capture (4);
  if (violations != 3)
    __builtin_abort ();
}
