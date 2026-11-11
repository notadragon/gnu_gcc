// P3100: with the [[noreturn]] check configured, constant evaluation is
// unchanged: a [[noreturn]] constexpr function that throws is usable in a
// constant expression, and one that returns is still rejected by the
// evaluator.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

[[noreturn]] constexpr void nr (int x) { if (x > 0) return; throw x; }
constexpr int f (int x) { try { nr (x); } catch (int) { return 1; } return 0; }	// { dg-error "noreturn... call returns" }
static_assert (f (0) == 1);
constexpr int g = f (1);	// { dg-message "in .constexpr. expansion" }
// { dg-prune-output "has a .return. statement" }
