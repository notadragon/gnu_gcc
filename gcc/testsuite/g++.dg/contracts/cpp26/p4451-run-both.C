// p4451-run.C under -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451 -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p4283" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4451-run.C"
