// p4451-run-forms.C under -fcontracts-p3400 alone (the P3400 label), with
// ignore configured.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4451-run-forms.C"
