// p4451-callerside-seq.C with the checks outlined
// (-fcontract-checks-outlined), under -fcontracts-p4451 alone: the
// caller-side checks are made by the caller's wrapper the same way.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p4451 -fcontract-checks-outlined" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4451-callerside-seq.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p4451-callerside-seq.C"
