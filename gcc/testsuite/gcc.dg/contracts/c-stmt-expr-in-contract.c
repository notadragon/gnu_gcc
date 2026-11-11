/* A GNU statement expression that declares a local is accepted in a _Pre or
   _Post predicate: the declaration inside it does not disturb the replay of
   the enclosing function's contract tokens, nor its parameters' scope.  (On
   a prototype it is rejected: GCC-617, open-bug-c-stmt-expr-in-prototype.c.)

   Mirror: clang/test/Contracts/C/c-stmt-expr-in-contract.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

int f (int x) _Pre (__extension__ ({ int t = x; t > 0; })) { return x; }
int g (int x) _Post (r: __extension__ ({ int t = r; t >= 0; })) { return x; }
int h (int x) _Pre (__extension__ ({ int t = x; t > 0; }))
	      _Post (r: __extension__ ({ int u = r; u >= 0; }))
{ return x; }
