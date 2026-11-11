// A P3595 "C++"-linkage selector name is a qualified name: an empty component
// other than a leading "::" is rejected when the configuration is parsed.
//
// Mirror: clang/test/Contracts/p3595-dynamic-bad-name.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-bad-name.json" }
// The diagnostics point into the JSON file, so match them with dg-regexp.
// { dg-regexp {[^\n]*p3595-dynamic-bad-name\.json: In JSON value '/0/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name\.json:3:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name\.json: In JSON value '/1/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name\.json:5:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name\.json: In JSON value '/2/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name\.json:7:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }

void f (int x) pre (x > 0) {}
