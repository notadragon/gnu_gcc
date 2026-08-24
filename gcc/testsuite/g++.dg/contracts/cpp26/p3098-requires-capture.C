// P3098+P4283: a postcondition carrying BOTH a requires-clause and a capture,
// in the wording-specified order `post requires(C) [captures] (result: pred)`
// (see P4283 overview).
//
// Previously BUG-4: GCC misparsed the combination -- the requires-clause
// constraint parse greedily consumed the following `[captures]` as a subscript
// expression, so the capture name was "not declared" and the result-name
// introducer misparsed.  Now, in a contract requires-clause, a `[` after the
// constraint opens the capture list (mirroring how `(` opens the predicate).
// See testing-gap-catalogue.md section 10.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontracts-p4283 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <concepts>

static int viol = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++viol; }

// Requires satisfied (integral): capture built, predicate holds.
template <typename T>
T same(T x) post requires(std::integral<T>) [old = x] (r: r == old) { return x; }

// Requires satisfied, predicate false -> violation.  Also proves 'old' was
// actually captured (r == old would spuriously hold if old were uninitialized
// and equal to r).
template <typename T>
T bump(T x) post requires(std::integral<T>) [old = x] (r: r == old) { return x + 1; }

// Requires unsatisfied (non-integral): the whole contract is discarded.
template <typename T>
T disc(T x) post requires(std::integral<T>) [old = x] (r: r == old) { return x + 1; }

int main() {
  viol = 0;
  (void) same(5);     // 5 == 5 -> holds
  if (viol != 0) __builtin_abort();
  (void) bump(5);     // 6 == 5 -> violation (requires satisfied)
  if (viol != 1) __builtin_abort();
  (void) disc(5.0);   // non-integral -> contract discarded, no check
  if (viol != 1) __builtin_abort();
}
