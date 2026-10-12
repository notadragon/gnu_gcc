// A P3595 dynamic selector name is emitted as a symbol (C linkage) or
// mangled into one (C++ linkage), so it must be an identifier, or
// identifiers separated by "::" for C++; anything else is rejected when the
// configuration is parsed.  A template-id, an operator-function-id, a
// parameter list, a component starting with a digit or containing a space,
// and an empty or qualified C name used to reach the assembler as an
// invalid symbol, or crash the compiler (GCC-671).  The last two entries
// are valid names, with digits, '$' and '_' where an identifier allows
// them, and are not diagnosed.  p3595-dynamic-bad-name.C covers empty
// components.
//
// Mirror: clang/test/Contracts/p3595-dynamic-bad-name-chars.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-bad-name-chars.json" }
// The diagnostics point into the JSON file, so match them with dg-regexp.
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/0/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:2:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/1/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:3:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/2/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:4:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/3/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:5:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/4/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:6:[0-9]+: error: 'name' in 'dynamic' is not a valid qualified name[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/5/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:7:[0-9]+: error: 'name' in 'dynamic' is not a valid identifier[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/6/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:8:[0-9]+: error: 'name' in 'dynamic' is not a valid identifier[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/7/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:9:[0-9]+: error: 'name' in 'dynamic' is not a valid identifier[^\n]*} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json: In JSON value '/8/output/dynamic/name'} }
// { dg-regexp {[^\n]*p3595-dynamic-bad-name-chars\.json:10:[0-9]+: error: 'name' in 'dynamic' is not a valid identifier[^\n]*} }

void f (int x) pre (x > 0) {}
