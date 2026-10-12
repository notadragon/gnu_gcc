// P3099: a diagnostic message in a template that is not a string literal --
// type-dependent, or a non-dependent class type -- is kept as written and
// extracted when the template is instantiated, as a static_assert message
// is; redeclarations still compare it.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <string_view>

struct M { constexpr std::size_t size () const { return 3; }
	   constexpr const char *data () const { return "abc"; } };
struct N { constexpr std::size_t size () const { return 2; }
	   constexpr const char *data () const { return "xy"; } };

const char *last;
void handle_contract_violation (const std::contracts::contract_violation &v)
{ last = v.message (); }

template <class T> void f (int x) pre (x > 0, T{});
template <class T> void f (int x) pre (x > 0, T{}) {}
template <class T> void g (int x) { contract_assert (x > 0, T{}); }
template <class T> int h (T x) pre (x > 0, M{}) { return 1; }
template <class T> void k (T x) { contract_assert (x > 0, std::string_view ("d")); }

static bool is (const char *s) { return last && std::string_view (last) == s; }

int main ()
{
  f<M> (0);
  if (!is ("abc")) __builtin_abort ();
  f<N> (0);
  if (!is ("xy")) __builtin_abort ();
  g<N> (0);
  if (!is ("xy")) __builtin_abort ();
  h (0);
  if (!is ("abc")) __builtin_abort ();
  k (0);
  if (!is ("d")) __builtin_abort ();
}
