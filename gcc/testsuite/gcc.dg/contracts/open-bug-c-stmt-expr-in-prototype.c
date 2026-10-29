/* Watch test for an open bug, GCC-617 in this repository's open-issues/: a
   GNU statement expression in a contract on a prototype is rejected ("braced-
   group within expression allowed only inside a function"), because the
   prototype's predicates are parsed, to be diagnosed and discarded, outside
   any function.  The same predicate on a definition is accepted
   (c-stmt-expr-in-contract.c).

   Mirror: clang/test/Contracts/C/c-stmt-expr-in-contract.c in the
   llvm_llvm-project fork (the prototype line, which passes).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

int h (int x) _Pre (__extension__ ({ int t = x; t > 0; }));	/* { dg-bogus "braced-group" "GCC-617" { xfail *-*-* } } */
