// P3098: Outlined contracts — capture init exception with observe semantic.
// Verifies struct-passing handles exceptions correctly in __pre_fn.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdlib>

int predicate_count = 0;
bool count_pred() { ++predicate_count; return true; }

struct Bad {
  Bad(int) { throw 42; }
  Bad(const Bad&) { throw 42; }
  ~Bad() {}
};

// Capture init throws — predicate must be skipped.
int f(int i)
  post [b = Bad(1)] (r: count_pred())
{
  return i;
}

// Normal case — outlined captures work.
int g(int i)
  post [old_i = i] (r: r > old_i)
{
  return i + 1;
}

int main() {
  predicate_count = 0;
  int result = f(10);
  if (predicate_count != 0)
    __builtin_abort ();
  if (result != 10)
    __builtin_abort ();

  if (g(5) != 6)
    __builtin_abort ();
}
// { dg-output "contract violation in function int f.int. at .*(\n|\r\n|\r)" }
// { dg-output ".assertion_kind: post_capture, semantic: observe, mode: evaluation_exception.*(\n|\r\n|\r)" }
