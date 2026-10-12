// GCC-683 (PR65174): noexcept of a delete-expression whose operand is a null
// pointer constant ignores a potentially-throwing destructor.
// Expected: clean.  -std=c++11
struct D { ~D () noexcept (false); };
D *p;

static_assert (!noexcept (delete p), "control");
static_assert (!noexcept (delete (D *) 0), "(D *) 0");
static_assert (!noexcept (delete static_cast<D *> (nullptr)), "nullptr");
