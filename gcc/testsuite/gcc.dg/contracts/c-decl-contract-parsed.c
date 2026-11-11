/* The contracts on a C function declaration that is not a definition have no
   effect (P4299), but they are parsed, with the parameters and the result
   name in scope, so an ill-formed predicate is diagnosed (DECISIONS.md K7).

   Mirror: clang/test/Contracts/C/c-predicate-conditional-expression.c in the
   llvm_llvm-project fork (the declaration lines).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

int g (int x) _Pre (x, 1);          /* { dg-error "expected" } */
int h (int x) _Post (r: r, 1);      /* { dg-error "expected" } */
int k (int x) _Pre (x = 1);         /* { dg-error "expected" } */
int u (int x) _Pre (nosuch_name);   /* { dg-error ".nosuch_name. undeclared" } */
int ok1 (int x) _Pre (x > 0);       /* OK */
int ok2 (int x) _Post (r: r > x);   /* OK */
int ok2 (int x) { return x + 1; }   /* OK, the prototype's _Post is not kept */
