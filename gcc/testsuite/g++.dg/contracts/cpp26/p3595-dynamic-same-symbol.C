// P3595: two config entries whose DISTINCT name strings resolve to the SAME
// selector symbol, each with provideweak (the default).  Entry A names the
// selector as a "C++" qualified name "mylib::sel"; entry B names the SAME
// symbol verbatim as its "C"-linkage mangled spelling "_ZN5mylib3selEv"
// (the Itanium mangling of mylib::sel()).  The two distinct name strings are
// distinct keys in the per-name decl cache, so each entry independently
// schedules a weak definition of the same final symbol.  The compiler must
// collapse these to at MOST ONE weak definition, keyed on the final assembler
// name -- emitting two would be a duplicate-symbol link error.  This test
// exercises that assembler-name backstop: it links (proving the dedup) and
// runs.  With no user selector, the single weak default (observe) drives both
// the "safety"-group contract (resolves through entry A) and the plain
// contract (resolves through entry B).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-same-symbol.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Resolves through entry A ("safety" group -> "C++" name mylib::sel).
void f_safety(int x) pre<"safety"group>(x > 0) { }

// Resolves through entry B (catch-all -> "C" name _ZN5mylib3selEv, the same
// symbol as mylib::sel).
void f_plain(int x) pre(x > 0) { }

int main() {
  // Both drive "observe" (the shared weak default): handler runs, continues.
  f_safety(-1);
  if (violations != 1) __builtin_abort();
  f_plain(-1);
  if (violations != 2) __builtin_abort();

  // Non-violating calls do nothing.
  f_safety(1);
  f_plain(1);
  if (violations != 2) __builtin_abort();
}
