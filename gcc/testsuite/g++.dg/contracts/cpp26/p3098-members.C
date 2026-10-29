// P3098: Postcondition captures on member functions (deferred parsing).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

struct Counter {
  int value = 0;

  void increment()
    post [old_val = value] (value == old_val + 1)
  {
    ++value;
  }

  int add(int n)
    post [old_val = value, n] (r: r == old_val + n)
  {
    value += n;
    return value;
  }
};

int main() {
  Counter c;
  c.increment();
  assert(c.value == 1);
  int r = c.add(5);
  assert(r == 6);
  assert(c.value == 6);
}
