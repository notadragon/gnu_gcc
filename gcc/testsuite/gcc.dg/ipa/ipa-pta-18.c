/* { dg-do run } */
/* { dg-options "-O2 -fipa-pta -fdump-ipa-pta2-details -fdump-tree-fre3-details" } */

static int __attribute__((noipa))
foo (int *p, int *q)
{
  *p = 1;
  *q = 0;
  return *p;
}

extern void abort (void);

int main()
{
  int a, b;
  if (foo (&a, &b) != 1)
    abort ();
  return 0;
}

/* Verify that the call of the noipa function foo is modeled as a call of
   an unknown function, so its arguments do not flow into its parameters
   and *p and *q in foo are not disambiguated.  */

/* { dg-final { scan-ipa-dump-not "foo.arg0 = &a" "pta2" } } */
/* { dg-final { scan-ipa-dump-not "foo.arg1 = &b" "pta2" } } */
/* { dg-final { scan-ipa-dump "foo.arg0 = &NONLOCAL" "pta2" } } */
/* { dg-final { scan-tree-dump-not "Replaced \\\*p_2\\\(D\\\) with 1" "fre3" } } */
