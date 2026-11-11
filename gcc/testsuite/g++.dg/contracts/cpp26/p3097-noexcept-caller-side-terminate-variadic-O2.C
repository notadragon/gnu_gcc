// p3097-noexcept-caller-side-terminate.C for the C-variadic function, at -O2.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce -O2 -DCASE=1" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3097-noexcept-caller-side-terminate.C"
