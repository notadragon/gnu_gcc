// A malformed line range in a P3595 "location" match criterion is diagnosed
// against the configuration source and its entry skipped
// (p3595-config-location-error.C covers the same ground as a feature test).
//
// Mirror: clang/test/Contracts/malformed-location-range.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/malformed-location-range.json" }
//
// The diagnostics point into the JSON file, so match them with dg-regexp.
// { dg-regexp {[^\n]*malformed-location-range\.json: In JSON value '/0/match/location'} }
// { dg-regexp {[^\n]*malformed-location-range\.json:[0-9]+:[0-9]+: error: malformed line range 'err.cpp:10x' in 'location'[^\n]*} }
// { dg-regexp {[^\n]*malformed-location-range\.json: In JSON value '/1/match/location'} }
// { dg-regexp {[^\n]*malformed-location-range\.json:[0-9]+:[0-9]+: error: malformed line range 'err.cpp:1-2z' in 'location'[^\n]*} }
// { dg-regexp {[^\n]*malformed-location-range\.json: In JSON value '/2/match/location'} }
// { dg-regexp {[^\n]*malformed-location-range\.json:[0-9]+:[0-9]+: error: malformed line range 'err.cpp:5-' in 'location'[^\n]*} }

int f (int x) { return x; }
