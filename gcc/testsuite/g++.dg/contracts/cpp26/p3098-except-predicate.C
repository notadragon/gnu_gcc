// P3098: Exception during predicate evaluation — captures still destroyed.
// With observe semantic: the predicate exception triggers the violation handler,
// and captures must still be properly destroyed.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdlib>

int destruct_count = 0;

struct Tracker {
  int id;
  Tracker(int i) : id(i) {}
  Tracker(const Tracker& o) : id(o.id) {}
  ~Tracker() { ++destruct_count; }
};

bool throwing_predicate(int) {
  throw 99;
}

int f(int i)
  post [t = Tracker(1)] (r: throwing_predicate(r))
{
  return i;
}

int main() {
  destruct_count = 0;
  int result = f(10);
  // Predicate threw — violation handler called (observe returns).
  // Capture must still be destroyed after predicate exception.
  if (destruct_count != 1)
    __builtin_abort ();
  if (result != 10)
    __builtin_abort ();
}
// { dg-output "contract violation in function int f.int. at .*(\n|\r\n|\r)" }
// { dg-output ".assertion_kind: post, semantic: observe, mode: evaluation_exception.*(\n|\r\n|\r)" }
