/* With -fcontracts-p4299, the compiler predefines __gcc_contracts_p4299
   (P4299's keywords need no header) and <contracts.h> defines
   __STDC_VERSION_CONTRACTS_H__, the C23 convention for a header's version
   (notadragon_wg21 DECISIONS.md A8, G1).  Both carry the date of the
   implemented revision until P4299 is adopted, so only check that they are
   positive.  c-feature-macros-without-p4299.c is the negative.

   Mirror: CLANG-479 in the llvm_llvm-project fork.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

#if !defined (__gcc_contracts_p4299) || __gcc_contracts_p4299 <= 0
#error "__gcc_contracts_p4299"
#endif

#ifdef __STDC_VERSION_CONTRACTS_H__
#error "__STDC_VERSION_CONTRACTS_H__ before <contracts.h>"
#endif

#include <contracts.h>

#if !defined (__STDC_VERSION_CONTRACTS_H__) || __STDC_VERSION_CONTRACTS_H__ <= 0
#error "__STDC_VERSION_CONTRACTS_H__"
#endif
