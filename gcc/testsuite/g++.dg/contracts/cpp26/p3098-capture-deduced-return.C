// A postcondition capture of a function template with a deduced return type
// and a result name.  The predicate is substituted as if in a template until
// the return type is deduced; the captures were too, so an initializer
// naming a member of a dependent object (`t.m') stayed a template tree: a
// segfault in the gimplifier for a pack capture, and for a scalar one a
// capture of dependent decltype type ("incomplete type", then an ICE behind
// the error) (GCC-477).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

struct S { int m; };

template <class T>
auto a (T t) post [c = t.m] (r: c > 0 && r == c) { return t.m; }
template <class T>
auto b (T t) post [c = t.m] (r: true) { return 0; }
template <class... T>
auto p (T... t) post [...c = t.m] (r: ((c > 0) && ...) && r == 0) { return 0; }
template <class... T>
auto q (T... t) post [...c = t.m + 1] (r: ((c > 1) && ...)) { return 0; }

template <class C>
struct X
{
  template <class... T>
  auto g (T... t) post [...c = t.m] (r: ((c > 0) && ...));
};
template <class C>
template <class... T>
auto X<C>::g (T... t) post [...c = t.m] (r: ((c > 0) && ...)) { return 0; }

int
main ()
{
  S one{1}, zero{0};
  X<int> x;
  a (one); b (one); p (one, one); q (one); x.g (one, one);
  if (violations != 0)
    __builtin_abort ();
  a (zero); p (one, zero); q (zero, one); x.g (zero);
  if (violations != 4)
    __builtin_abort ();
}
