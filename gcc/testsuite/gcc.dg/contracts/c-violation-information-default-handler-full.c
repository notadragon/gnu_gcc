/* Under -fcontract-violation-information=full the default violation
   handler prints the function, file, line and comment, as it always has;
   c-violation-information-default-handler.c is the same under none.

   Mirror, in the llvm_llvm-project fork:
   clang/test/Contracts/C/c-violation-information-default-handler-full.c.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe -fcontract-violation-information=full" } */

void pre_f (int x) _Pre (x > 0) { }
void assert_f (int x) { _ContractAssert (x > 0); }

int
main (void)
{
  pre_f (0);
  assert_f (0);
  return 0;
}

/* { dg-output "^contract violation in function pre_f at \[^\n\r\]*c-violation-information-default-handler-full.c:10: x > 0(\n|\r\n|\r)" } */
/* { dg-output "contract violation in function assert_f at \[^\n\r\]*c-violation-information-default-handler-full.c:11: x > 0(\n|\r\n|\r)$" } */
