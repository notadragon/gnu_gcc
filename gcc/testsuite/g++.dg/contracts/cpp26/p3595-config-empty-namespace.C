// "namespace": "" in a P3595 configuration entry is an error, in "match" and
// in "match"/"caller" alike.
//
// Mirror: clang/test/Contracts/p3595-config-empty-namespace.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-empty-namespace.json" }
// { dg-error ".namespace. in .caller. must not be empty" "" { target *-*-* } 0 }
// { dg-error ".namespace. must not be empty" "" { target *-*-* } 0 }
// { dg-prune-output "In JSON value" }

int f (int x) pre (x > 0) { return x; }
