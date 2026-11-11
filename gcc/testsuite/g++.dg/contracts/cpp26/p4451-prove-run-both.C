// p4405-paper-analysis-terminating-run.C under -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label): the
// terminating precondition is still enforced when built to ignore contract
// assertions, and a configured analysis semantic of diagnose, which
// terminating does not allow, falls back to ignore.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "enforce terminates after the violation" }

#include "p4405-paper-analysis-terminating-run.C"

// { dg-output "violation: p >= 0 && p <= 100" }
