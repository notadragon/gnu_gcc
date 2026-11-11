// P3100: with the function-try-block handler check configured, constant
// evaluation is unchanged: a constructor whose handler is not entered stays
// usable in a constant expression, and a handler reading a member that was
// never initialized is still rejected by the evaluator.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:except.handle.handler.ctor.dtor:enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct A {
  constexpr A (int v) try : x (v == 1 ? throw 1 : v) { }
  catch (int) { throw y; }	// { dg-error "accessing uninitialized member" }
  int x;
  int y = 42;
};
constexpr int f (int v) { try { A a (v); return a.x; } catch (int e) { return e; } }
static_assert (f (5) == 5);
constexpr int g = f (1);	// { dg-message "in .constexpr. expansion" }
