// P4451: <contracts> as a header unit declares
// std::contracts::labels::terminating for its importers
// (contract-p4451-hu-p3400_b.C).  Under -fcontracts-p3400 alone (the P3400 label).
// { dg-do compile { target c++26 } }
// { dg-additional-options {-x c++-system-header contracts -fmodules -fcontracts-p3400} }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }
// { dg-prune-output {linker input file unused} }

NO DO NOT COMPILE
