// post-capture-noexcept.C with the checks outlined.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=noexcept_observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "post-capture-noexcept.C"
