// P3098: Postcondition captures on noexcept functions.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

int f(int i) noexcept
  post [old_i = i] (r: r > old_i)
{
  return i + 1;
}

int g(int a, int b) noexcept
  post [a, b] (r: r == a + b)
{
  int result = a + b;
  a = 0;
  b = 0;
  return result;
}

void h(int i) noexcept
  post [old_i = i] (old_i > 0)
{
}

int main() {
  assert(f(10) == 11);
  assert(g(3, 4) == 7);
  h(5);
}
