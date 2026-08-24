// P3098: Postcondition captures — basic runtime behavior.
// Captures hold values from call time, not return time.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

// Init-capture records value at call time.
int f(int i)
  post [old_i = i] (r: r > old_i)
{
  i = 0;  // modify param — capture should still hold original
  return 42;
}

// Multiple captures.
int g(int a, int b)
  post [a, b] (r: r == a + b)
{
  int result = a + b;
  a = 0;
  b = 0;
  return result;
}

// Init-capture from expression (not param).
int counter = 0;
int get_value() { return ++counter; }

int h()
  post [start = get_value()] (r: r > start)
{
  return get_value() + 100;
}

// Capture with return name.
int add_one(int x)
  post [x] (r: r == x + 1)
{
  return x + 1;
}

int main() {
  assert(f(10) == 42);
  assert(g(3, 4) == 7);
  h();
  assert(add_one(5) == 6);
}
