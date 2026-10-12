/* <contracts.h> names evaluation semantics 5 (assume), 6 (noexcept_observe)
   and 7 (noexcept_enforce) as well as 1-4: stdc_contract_violation_semantic()
   can return them (a handler shared with C++, or the P4298 extension in C).
   P4299 does not fix these names; they follow the STDC_CONTRACT_* pattern.

   Mirror: clang/test/Contracts/C/contracts-h-semantic-constants.c in the
   llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#include <contracts.h>

_Static_assert (STDC_CONTRACT_ASSUME == 5, "assume");
_Static_assert (STDC_CONTRACT_NOEXCEPT_OBSERVE == 6, "noexcept_observe");
_Static_assert (STDC_CONTRACT_NOEXCEPT_ENFORCE == 7, "noexcept_enforce");
