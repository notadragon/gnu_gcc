// D4405R0, "Previous Assertions and Undefined Behavior" (GCC-1003): a
// function's own precondition whose predicate calls a function is a premise
// in its body.  The body's call of get, whose precondition is what each
// use's precondition says, is proven in use1 (the predicate calls get,
// which has a precondition of its own), use2 (an inline accessor) and use3
// (the member read directly).  Every evaluation is prove, so each own
// precondition is a premise that is not guaranteed and the proofs are
// conditional (p4405-own-pre-call-premise-sound.C has sound ones).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=prove" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

struct C {
  int *p;
  int *get () const pre (p) { return p; }	// { dg-bogus "the predicate of the contract assertion" }
  int *raw () const { return p; }
};

void use1 (C &c) pre (c.get () != nullptr) { c.get (); }	// { dg-bogus "cannot be proven" }
void use2 (C &c) pre (c.raw () != nullptr) { c.get (); }	// { dg-bogus "cannot be proven" }
void use3 (C &c) pre (c.p != nullptr) { c.get (); }		// { dg-bogus "cannot be proven" }

// { dg-final { scan-tree-dump {\(line 19\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 20\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 21\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump-not {: unproven} "contract_shadow" } }
