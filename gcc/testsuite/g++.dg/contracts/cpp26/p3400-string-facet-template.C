// P3400: a compute_message facet on a contract in a template is applied once,
// at instantiation, as it is outside a template.  It used to be evaluated in
// the template body, where a facet that reads its argument is not a constant
// expression.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <string_view>

const char *last;
void handle_contract_violation (const std::contracts::contract_violation &v)
{ last = v.message (); }

struct flip_t {
  using assertion_control_object = flip_t;
  constexpr const char *compute_message (const char *m) const
  { return (m && m[0] == 'A') ? "B" : "A"; }
};
constexpr flip_t flip{};

void f (int x) pre<flip> (x > 0, "A") {}
template <typename T> void g (T x) pre<flip> (x > 0, "A") {}

static bool is (const char *s) { return last && std::string_view (last) == s; }

int main ()
{
  f (0);
  if (!is ("B")) __builtin_abort ();
  g (0);			// applied once, not twice
  if (!is ("B")) __builtin_abort ();
}
