// P3595: an invalid inline contract configuration (-fcontract-configuration=)
// is diagnosed against <command-line>, eagerly, without needing any contract
// assertion in the translation unit.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options {-fcontract-configuration=not-json} }
//
// { dg-regexp {<command-line>:[0-9]+:[0-9]+: error: invalid JSON in contract configuration:[^\n]*} }

int f (int x) { return x; }
