// P3098: Function body throws — captures destroyed, predicate not evaluated.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

int destruct_count = 0;

struct Tracker {
  Tracker(int) {}
  Tracker(const Tracker&) {}
  ~Tracker() { ++destruct_count; }
};

int f(int i)
  post [a = Tracker(1), b = Tracker(2)] (true)
{
  throw 99;
  return i;
}

int main() {
  destruct_count = 0;
  try {
    f(10);
  } catch (int e) {
    assert(e == 99);
  }
  // Both captures should be destroyed during unwinding.
  assert(destruct_count == 2);
}
