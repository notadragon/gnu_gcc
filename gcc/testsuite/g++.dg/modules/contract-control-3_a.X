// P3400: <contracts> as a header unit carries its contract_control
// using-directive (D3400R6 [module.interface]), and the names it
// nominates stay out of ordinary lookup (contract-control-3_b.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options {-x c++-system-header contracts -fmodules -fcontracts-p3400} }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }
// { dg-prune-output {linker input file unused} }

NO DO NOT COMPILE
