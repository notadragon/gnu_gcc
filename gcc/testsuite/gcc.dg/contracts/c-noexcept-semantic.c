/* The P4298 noexcept semantics are supported in C as an extension:
   -fcontracts-p4298 with -fcontract-evaluation-semantic=noexcept_observe in
   a C compile is accepted without a warning, and the handler sees
   STDC_CONTRACT_NOEXCEPT_OBSERVE.

   Mirror: clang/test/Contracts/C/c-noexcept-semantic.c in the
   llvm_llvm-project fork.  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe" } */
/* { dg-bogus "valid for C\\+\\+/ObjC\\+\\+ but not for C" "" { target *-*-* } 0 } */

#include <contracts.h>

static int handler_called = 0;
static int a_semantic = 0;

void handle_contract_violation (const contract_violation_t *cv)
{
  handler_called++;
  a_semantic = stdc_contract_violation_semantic (cv);
}

int f (int x) _Pre (x > 0) { return x; }

int main (void)
{
  f (0);
  if (handler_called != 1)
    __builtin_abort ();
  if (a_semantic != STDC_CONTRACT_NOEXCEPT_OBSERVE)
    __builtin_abort ();
  return 0;
}
