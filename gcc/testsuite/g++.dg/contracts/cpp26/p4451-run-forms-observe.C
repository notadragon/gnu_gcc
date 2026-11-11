// p4451-run-forms.C under -fcontracts-p4451 alone, with observe configured.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4451-run-forms.C"
