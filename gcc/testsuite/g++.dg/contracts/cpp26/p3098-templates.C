// P3098: Postcondition captures in function templates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

template <typename T>
T increment(T x)
  post [old_x = x] (r: r > old_x)
{
  return x + 1;
}

template <typename T>
T add(T a, T b)
  post [a, b] (r: r == a + b)
{
  T result = a + b;
  a = T{};
  b = T{};
  return result;
}

int main() {
  assert(increment(5) == 6);
  assert(increment(3.14) > 3.14);
  assert(add(3, 4) == 7);
  assert(add(1.5, 2.5) == 4.0);
}
