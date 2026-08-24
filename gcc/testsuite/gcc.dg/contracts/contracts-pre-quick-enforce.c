/* Test _Pre with quick_enforce semantic: traps without calling handler.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=quick_enforce" } */
/* { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } } */
/* { dg-shouldfail "" } */

int guarded(int x) _Pre(x > 0)
{
  return x;
}

int main(void)
{
  guarded(-1);
  /* Should not reach here: quick_enforce traps.  */
  return 0;
}
