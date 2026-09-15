// Companion to contract-source-location-alias.C, for the second shape in
// PR127255.  A union reaches a DIFFERENT stock crash site -- it satisfies the
// tree check in contracts.cc and then dies in build_source_location_impl
// (cp-gimplify.cc) on the error node produced by the missing __impl member --
// so guarding only the first site would leave this one crashing.  Kept as its
// own test for that reason.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

namespace std { union source_location {}; }

void foo () pre (true) {}
