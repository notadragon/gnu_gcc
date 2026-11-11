// D4405 ("The Analysis Evaluation") and notadragon_wg21 DECISIONS.md E186
// (a): a diagnose warning whose analysis evaluation is in a system header
// is shown when the contract assertion is the program's -- a closure's
// precondition analyzed where a system header's forwarding template calls
// it -- located at that call, with the instantiation trail into this file.
// A warning for an assertion that is itself in a system header stays
// hidden without -Wsystem-headers: sys_caller's call of sys_callee, and the
// header's own closure passed to sys_invoke.  A call of sys_callee made
// here is located here and warned about as any other.  At proof level 0, so
// nothing here depends on inlining; p4405-analysis-indirect-lambda.C has
// the libstdc++ form at -O2, and p4405-analysis-system-header-sys.C shows
// the hidden warnings with -Wsystem-headers.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 -fcontracts-prove-optimize=0" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4405-analysis-system-header.h"

void
use (int n)
{
  auto ld = [] (int x) pre<always_diagnose> (x > 0) {};	// { dg-message "the predicate of the contract assertion" }
  sys_invoke (ld, n);				// { dg-message "required from here" }
  sys_caller (n);
  sys_lambda_caller (n);
  sys_callee (n);				// { dg-warning "contract assertion cannot be proven" }
}

// The closure's precondition, analyzed at sys_invoke's call of it (line 20
// of the header).  The header's own closure, analyzed there too, and
// sys_callee's precondition analyzed at sys_caller's call (line 32) are not
// reported.
// { dg-regexp {[^\n]*p4405-analysis-system-header\.h:20:[0-9]+: warning: contract assertion cannot be proven \[-Wcontract-unproven\]\n} }
// { dg-message "In instantiation of" "" { target *-*-* } 0 }
// { dg-message "the predicate of the contract assertion" "" { target *-*-* } 0 }
