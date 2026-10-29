// Precompiling a header that defines a function with a checked contract:
// the PCH writer walks the cache of __cxa_contract_violation_* entry points,
// which must be keyed on something it can relocate (an identifier, not a
// string).
//
// Compiled as a header: `dg-do assemble' puts the options before the source,
// so `-x c++-header' applies to it (as in g++.dg/header.C).
// { dg-do assemble { target c++26 } }
// { dg-additional-options "-x c++-header -fcontracts -fcontract-evaluation-semantic=enforce" }

int nf (int x) pre (x > 0) { return x; }
int no (int x) pre (x > 0) post (r: r > 0) { contract_assert (x != 3); return x; }
