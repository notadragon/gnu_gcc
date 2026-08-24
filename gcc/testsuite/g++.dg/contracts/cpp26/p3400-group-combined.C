// P3400: Test group identification labels combined with operator|.
// Combined labels inherit group_names from both sides.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontract-group-evaluation-semantic=safety:observe -fcontract-group-evaluation-semantic=audit:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::labels::operator|;
using std::contracts::labels::review;

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Two group labels combined — contract is in both groups
void f_safety_audit(int x)
  pre<("safety"group | "audit"group)>(x > 0)
{ }

// Group label combined with non-group label (review has compute_semantic
// but no group_names).  The combined label is in "safety" group only.
void f_safety_review(int x)
  pre<("safety"group | review)>(x > 0)
{ }

// Non-group label combined with group label (order reversed).
void f_review_safety(int x)
  pre<(review | "safety"group)>(x > 0)
{ }

// Two non-group labels — no group membership
void f_review_review(int x)
  pre<(review | review)>(x > 0)
{ }

// Group combined with group, only first group has config
// (audit:observe configured, "other" not configured)
void f_audit_other(int x)
  pre<("audit"group | "other"group)>(x > 0)
{ }

// Group combined with group, only second group has config
void f_other_audit(int x)
  pre<("other"group | "audit"group)>(x > 0)
{ }

int main() {
  // safety | audit: both groups configured to observe
  // "safety" matches first → observe
  f_safety_audit(-1);
  if (violations != 1) __builtin_abort();

  // safety | review: "safety" group → observe (review also transforms
  // semantic, but group config fires first at the config level)
  f_safety_review(-1);
  if (violations != 2) __builtin_abort();

  // review | safety: same groups, same result
  f_review_safety(-1);
  if (violations != 3) __builtin_abort();

  // review | review: no groups, falls through to default.
  // review transforms enforce→observe via compute_semantic.
  // So this should observe.
  f_review_review(-1);
  if (violations != 4) __builtin_abort();

  // audit | other: "audit" matches config → observe
  f_audit_other(-1);
  if (violations != 5) __builtin_abort();

  // other | audit: "audit" also matches config → observe
  f_other_audit(-1);
  if (violations != 6) __builtin_abort();
}
