// stmt-expr-local-lambda-pre.C with the checks outlined: a statement
// expression's variable belongs to the outlined check function then.  The
// lambda in a function template is included: its outlined checks were put
// in a comdat group of their own as well as the operator()'s, which a
// checking build's verify_cgraph_node rejected (GCC-657).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "stmt-expr-local-lambda-pre.C"
