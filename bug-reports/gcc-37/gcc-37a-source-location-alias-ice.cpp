// GCC-37 / PR127255: a std::source_location that is not a class ICEs the
// contract machinery.
//
// Stock's get_contracts_source_location_impl_type looks the name up with
// lookup_std_type and then reaches for TYPE_FIELDS without checking that what
// came back is a class type.  An alias to a non-class -- or a union, which has
// no __impl member -- fails the tree check.
//
// Compile with: -std=c++26 -fcontracts
// Stock trunk: internal compiler error in get_contracts_source_location_impl_type
// This branch: accepted; the front end never looks the name up.
//
// The program is ill-formed by [namespace.std], so this is ice-on-invalid --
// but an ICE is still the wrong diagnosis.

namespace std { using source_location = int; }

void foo () pre (true) {}
