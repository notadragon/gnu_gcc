// P3098: A postcondition capture referenced in the predicate of a member
// function of a CLASS TEMPLATE.  This works for free function templates
// (p3098-templates.C) and members of non-template classes
// (p3098-member-captures.C), and now for class-template members too.
//
// Previously BUG-1: GCC wrongly rejected the capture use with "use of local
// variable with automatic storage from containing function" during class-
// template instantiation -- the postcondition predicate may reference its
// captures, whose context is the instantiated function.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++viol; }

template <typename T>
struct S {
  T bump (T x) post [old = x] (r: r == old + 1) { return x + 1; }
  T bad  (T x) post [old = x] (r: r == old + 5) { return x + 1; }  // violates
};

int main () {
  S<int> s;
  if (s.bump (1) != 2 || viol != 0) __builtin_abort ();
  (void) s.bad (1);
  if (viol != 1) __builtin_abort ();
}
