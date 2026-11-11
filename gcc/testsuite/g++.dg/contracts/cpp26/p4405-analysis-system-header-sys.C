// p4405-analysis-system-header.C with -Wsystem-headers: the warnings for
// the assertions that are themselves in the system header, hidden there,
// are shown -- the header's own closure analyzed at sys_invoke's call (line
// 20 of the header, with the program's closure) and sys_callee's
// precondition analyzed at sys_caller's call (line 32) -- so that test's
// silence is the system-header rule's, not a proof.  -Wno-literal-suffix and
// -Wno-pedantic keep the library's own extensions quiet.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O0 -Wsystem-headers -Wno-literal-suffix -Wno-pedantic -fcontracts-prove-optimize=0" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4405-analysis-system-header.h"

void
use (int n)
{
  auto ld = [] (int x) pre<always_diagnose> (x > 0) {};	// { dg-message "the predicate of the contract assertion" }
  sys_invoke (ld, n);				// { dg-message "required from here" }
  sys_caller (n);				// { dg-message "required from here" }
  sys_lambda_caller (n);
  sys_callee (n);				// { dg-warning "contract assertion cannot be proven" }
}

// { dg-regexp {[^\n]*p4405-analysis-system-header\.h:20:[0-9]+: warning: contract assertion cannot be proven \[-Wcontract-unproven\]\n} }
// { dg-regexp {[^\n]*p4405-analysis-system-header\.h:20:[0-9]+: warning: contract assertion cannot be proven \[-Wcontract-unproven\]\n} }
// { dg-regexp {[^\n]*p4405-analysis-system-header\.h:32:[0-9]+: warning: contract assertion cannot be proven \[-Wcontract-unproven\]\n} }
// { dg-message "the predicate of the contract assertion" "" { target *-*-* } 0 }
// { dg-message "In instantiation of" "" { target *-*-* } 0 }
