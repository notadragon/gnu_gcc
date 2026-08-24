// P3098: a postcondition capture whose initializer is a type-dependent
// expression -- e.g. a member access on a dependent function parameter,
// [c = x.val] -- must not crash.  unlowered_expr_type yields no type for such an
// initializer at parse time, so the capture's type is deferred (via decltype)
// and deduced at instantiation.  Regression: this previously segfaulted the
// parser (cp_type_quals on a null type) whenever the enclosing function was a
// template.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

struct S { int val; };

static int viol = 0;
void handle_contract_violation (const std::contracts::contract_violation &) {
  ++viol;
}

// Capture initializer x.val is a member access on the dependent parameter x.
template <class T>
void f (T x)
  post [c = x.val] (c >= 0)
{ }

// Also exercise the value being captured (not re-read from x) by mutating the
// object after entry -- the capture holds the entry-time value.
template <class T>
void g (T x)
  post [c = x.val] (c >= 0)
{ x.val = 999; }

int main ()
{
  viol = 0; f (S{5});   if (viol != 0) __builtin_abort ();  // c = 5  -> holds
  viol = 0; f (S{-3});  if (viol != 1) __builtin_abort ();  // c = -3 -> fails

  viol = 0; g (S{7});   if (viol != 0) __builtin_abort ();  // c = 7 (entry value)
  viol = 0; g (S{-1});  if (viol != 1) __builtin_abort ();  // c = -1 -> fails
}
