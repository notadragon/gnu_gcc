// GCC-37, second shape (PR127255 comment #1): std::source_location declared as
// a union rather than aliased away.  Same root cause, different tree check --
// a union reaches further into stock's lookup before failing on the missing
// __impl member.
//
// Compile with: -std=c++26 -fcontracts
// Stock trunk: internal compiler error
// This branch: accepted.

namespace std { union source_location {}; }

void foo () pre (true) {}
