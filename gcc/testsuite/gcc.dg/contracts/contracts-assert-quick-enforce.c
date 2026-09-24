/* Test _ContractAssert with quick_enforce: traps without calling handler.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-shouldfail "" } */

int main(void)
{
  int x = -1;
  _ContractAssert(x > 0);
  return 0;
}
