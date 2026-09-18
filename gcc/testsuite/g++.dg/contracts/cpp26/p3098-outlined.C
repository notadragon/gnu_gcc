// P3098: Postcondition captures with outlined contract checks.
// Captures are passed to __pre_fn and __post_fn via struct references.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

// Basic init-capture.
int f(int i)
  post [old_i = i] (r: r > old_i)
{
  return i + 1;
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

// No captures — uses outlined path normally.
int h(int x)
  pre (x > 0)
  post (r: r > 0)
{
  return x;
}

// Void function with captures.
int void_side = 0;
void v(int i)
  post [old_i = i] (void_side > old_i)
{
  void_side = i + 1;
}

// Interleaved precondition and captures.
int interleaved(int x)
  post [old_x = x] (r: r > old_x)
  pre (x >= 0)
{
  return x + 1;
}

// Mixed: one postcondition with captures, one without.
int mixed(int x)
  post [old_x = x] (r: r > old_x)
  post (r: r > 0)
{
  return x + 1;
}

// Only postcondition with captures, no precondition.
int post_only(int x)
  post [old_x = x] (r: r >= old_x)
{
  return x;
}

int main() {
  assert(f(10) == 11);
  assert(g(3, 4) == 7);
  assert(h(5) == 5);
  v(10);
  assert(void_side == 11);
  assert(interleaved(5) == 6);
  assert(mixed(5) == 6);
  assert(post_only(42) == 42);
}
