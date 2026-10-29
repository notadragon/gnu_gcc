// GCC-682: noexcept ignores a potentially-throwing default constructor run
// by an initializer list's array filler.  Expected: clean.  -std=c++20
struct S { S () noexcept (false); };
struct A { S s[2]; };
using S2 = S[2];

static_assert (!noexcept (A{}), "A{}");
static_assert (!noexcept (A{{}}), "A{{}}");
static_assert (!noexcept (S2{}), "S2{}");
