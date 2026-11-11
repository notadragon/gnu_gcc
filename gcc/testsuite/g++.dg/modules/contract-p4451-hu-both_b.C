// Consumer for contract-p4451-hu-both_a.X: "import <contracts>;" precedes
// the assertion-control-specifiers.
// { dg-additional-options "-fmodules -fcontracts-p3400 -fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

import <contracts>;

int f (int x) pre<terminating> (x > 0) post<terminating> (r : r > 0)
{
  contract_assert<terminating> (x > 1);
  return x;
}
