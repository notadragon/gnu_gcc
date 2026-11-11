// p4451-callerside-skip.C under -fcontracts-p3400 alone (the P3400
// label), with the caller-side checks of both preconditions asked for
// (p4451-callerside-skip-good.json).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-skip-good.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4451-callerside-skip.C"
