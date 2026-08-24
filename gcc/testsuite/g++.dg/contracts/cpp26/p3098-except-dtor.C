// P3098: Exception during capture destruction.
// Capture destructor throws during cleanup after predicate evaluation.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdlib>

int predicate_count = 0;
bool count_predicate() { ++predicate_count; return true; }

struct ThrowOnDestroy {
  int id;
  ThrowOnDestroy(int i) : id(i) {}
  ThrowOnDestroy(const ThrowOnDestroy& o) : id(o.id) {}
  ~ThrowOnDestroy() noexcept(false) { throw id; }
};

int f(int i)
  post [t = ThrowOnDestroy(1)] (r: count_predicate())
{
  return i;
}

int main() {
  predicate_count = 0;
  try {
    f(10);
  } catch (int e) {
    // Destructor threw during capture destruction.
    // Predicate must have been evaluated first.
    if (predicate_count != 1)
      __builtin_abort ();
    if (e != 1)
      __builtin_abort ();
    return 0;
  }
  // Should not reach here — destructor always throws.
  __builtin_abort ();
}
