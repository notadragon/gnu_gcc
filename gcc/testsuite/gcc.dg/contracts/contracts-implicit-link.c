/* Verify that -fcontracts-p4299 alone links the program: the driver must
   pull in libcontracts (which provides __c_contract_check_*), with no
   explicit -l/-L in the test.  This proves the flag pulls its dependency.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */

int f (int x) _Pre (x > 0) { return x; }

int
main (void)
{
  return f (1) == 1 ? 0 : 1;
}
