// A std::source_location that is not a class type must not disturb contract
// checking.  Stock trunk ICEs on both shapes (PR127255) because it reaches
// into the type's layout to build the violation object; this branch carries
// the source location as a POD field of the ABI data block and never looks the
// name up, so it is immune by construction.
//
// This pins that immunity: if the front end ever reacquires a
// lookup_std_type ("source_location") on the contract path, this test starts
// crashing.
//
// Both programs are ill-formed by [namespace.std] and no diagnostic is
// required, so accepting them is conforming.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

namespace std { using source_location = int; }

void foo () pre (true) {}
