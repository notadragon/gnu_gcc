/* An ignored precondition is still parsed, so an ill-formed predicate is
   diagnosed even under the ignore semantic (D4299).  Previously an ignored
   _Pre skipped parsing entirely, silently accepting invalid predicates
   (inconsistent with ignored _Post and _ContractAssert, and with C++).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=ignore" } */

int foo(int x) _Pre(undeclared_var > 0) /* { dg-error "undeclared" } */
{
  return x;
}
