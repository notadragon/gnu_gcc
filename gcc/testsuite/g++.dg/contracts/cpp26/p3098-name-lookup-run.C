// P3098: Runtime verification of name lookup rules.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdlib>

// Helper to observe capture values from within predicates.
// Function calls are allowed to have side effects.
int observed_value = -1;
bool record(int val) { observed_value = val; return true; }

// --- Test 1: Captures don't see each other (P3098 Section 4.2) ---
// [i_cap = 3, j_cap = i]: j_cap's initializer sees parameter i, not i_cap.
int test1(int i)
  post [i_cap = 3, j_cap = i] (r: record(j_cap))
{
  return 0;
}

// --- Test 2: Capture shadows outer variable (P3098 Section 4.2) ---
// post [b=b] (b) captures outer_b's call-time value (true), body sets it false.
// Predicate sees captured b (true), not modified outer_b (false).
bool outer_b = true;

bool test2_captured()
  post [b = outer_b] (b)
{
  outer_b = false;
  return true;
}

// --- Test 3: Capture from parameter, param modified in body ---
int test3(int i)
  post [old_i = i] (r: record(old_i))
{
  i = 999;
  return 0;
}

// --- Test 4: Multiple captures, independent init ---
int test4_a = -1, test4_b = -1;
bool record2(int a, int b) { test4_a = a; test4_b = b; return true; }

int test4(int x, int y)
  post [a = x, b = y] (r: record2(a, b))
{
  x = 0;
  y = 0;
  return 0;
}

// --- Test 5: Capture is NOT const (P3098 4.4.1) — mutation allowed ---
int test5(int i)
  post [old = i] (r: ++old > 0)
{
  return 1;
}

int main() {
  // Test 1: j_cap should be 42 (parameter value), not 3 (other capture).
  observed_value = -1;
  test1(42);
  if (observed_value != 42)
    __builtin_abort ();

  // Test 2: captured b preserves call-time outer_b (true).
  outer_b = true;
  test2_captured();
  // outer_b is now false (modified in body), but predicate passed (captured b was true).
  if (outer_b)
    __builtin_abort ();

  // Test 3: captured value preserved despite body modification.
  observed_value = -1;
  test3(77);
  if (observed_value != 77)
    __builtin_abort ();

  // Test 4: independent captures from different params.
  test4(10, 20);
  if (test4_a != 10 || test4_b != 20)
    __builtin_abort ();

  // Test 5: capture mutation in predicate is allowed.
  test5(5);
}
