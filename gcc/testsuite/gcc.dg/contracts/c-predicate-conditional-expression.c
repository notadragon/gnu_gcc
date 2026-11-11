/* A C contract predicate is a conditional-expression, as in C++, so a comma
   or assignment expression needs parentheses.  The same on a declaration
   that is not a definition is in c-decl-contract-parsed.c.

   Mirror: clang/test/Contracts/C/c-predicate-conditional-expression.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

int g2 (int x) _Pre (x, 1) { return x; }     /* { dg-error "expected" } */
int h2 (int x) _Post (r: r, 1) { return x; } /* { dg-error "expected" } */
int k2 (int x) _Pre (x = 1) { return x; }    /* { dg-error "expected" } */
int ok (int x) _Pre ((x, 1)) { return x; }

int
f (int x)
{
  _ContractAssert (x, 1);           /* { dg-error "expected" } */
  _ContractAssert (x = 1);          /* { dg-error "expected" } */
  _ContractAssert ((x, 1));
  return x;
}
