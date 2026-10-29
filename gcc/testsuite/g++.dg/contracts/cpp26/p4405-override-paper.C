// D4405R0, "Virtual Functions": the paper's example.  For each function
// B::f that D::f overrides, an override shadow of D::f's body takes
// B::f's preconditions as given and proves, in order, D::f's
// preconditions at entry, D::f's postconditions and B::f's postconditions
// at each return.  D::f's `x >= 0` follows from B::f's `x > 0`, and both
// postconditions from the body; E::f's `x > 10` does not follow, so the
// program is ill-formed, diagnosed at E::f's definition with the
// overridden function named, while B::f's `r > 0` is proven for E::f too.
// Nothing calls any of them, so no precondition has an analysis
// evaluation at a call; make() emits the vtables, and with them the
// overriders, which a translation unit that never compiles them would
// leave to the one that does (DECISIONS.md E150).  The paper's D::f
// declares `x` without const, which a postcondition naming it requires.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

struct B {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0);
};

struct D : B {
  int f(const int x) override	// { dg-bogus "cannot be proven" }
    pre(x >= 0)
    post(r: r > x)
  {
    return x + 1;
  }
};

struct E : B {
  int f(int x) override		// { dg-error "contract assertion cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre(x > 10)			// { dg-message "the predicate of the contract assertion" }
  {
    return x;
  }
};

B *make (bool d) { return d ? static_cast<B *> (new D) : new E; }

// In D::f's override shadow, D::f's precondition (line 26), its
// postcondition (line 27) and B::f's (line 21) -- conditional, since
// B::f's precondition, evaluated by the callers, may be ignored or
// observed there (DECISIONS.md E151) -- and D::f's postcondition in D::f's
// ordinary shadow; in E::f's, E::f's precondition (line 35) and B::f's
// postcondition.
// { dg-final { scan-tree-dump {\(line 26, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 27, override pair [0-9]+\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-times {\(line 21, override pair [0-9]+\): conditional} 2 "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 35, override pair [0-9]+\): unproven} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 27\): sound} "contract_shadow" } }
