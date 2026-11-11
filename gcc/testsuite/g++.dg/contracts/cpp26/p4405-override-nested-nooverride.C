// P4405, DECISIONS.md E169 (8), E157: p4405-override-nested.C with
// -fno-contracts-prove-override: D::f's preconditions are decided at once
// by folding and B::f's postconditions in D::f's ordinary shadow, with no
// premise from B::f's preconditions, so D1's and D5's nested chk calls
// are unproven too; D::f's postconditions get no override obligation.
//
// Mirror: clang/test/Contracts/p4405-override-nested-nooverride.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -O2 -fno-contracts-prove-override" }
// { dg-additional-options "-fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-analysis-semantic=prove" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-nested-diagnose.json" }

int chk (int v) pre (v != 0);	// { dg-message "precondition of a call in the predicate cannot be proven" }
				// { dg-message "precondition of a call in the predicate is always false" "" { target *-*-* } .-1 }
int chk (int) { return 1; }
namespace dd {
int dchk (int v) pre (v != 0);	// { dg-message "the predicate of the contract assertion" }
int dchk (int) { return 1; }
}

// Provable: B1::f's precondition makes x, hence r, nonzero.
struct B1 { virtual int f (int x) pre (x > 0) post (r: chk (r) == 1); };
struct D1 : B1 {
  int f (int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B1::f\\(int\\)'" }
  { return x; }
};

// Unprovable: nothing is known of r.
struct B2 { virtual int f (int x) post (r: chk (r) == 1); };
struct D2 : B2 {
  int f (int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B2::f\\(int\\)'" }
  { return x; }
};

// Always false: chk (0).
struct B3 { virtual int f (int x) post (r: chk (0) == 1); };
struct D3 : B3 {
  int f (int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B3::f\\(int\\)'" }
  { return x; }
};

// Diagnose: dd::dchk's precondition, unprovable, warned at its call in
// B4::f's postcondition, naming the pair; the outer is not affected
// (beyond being conditional).
struct B4 {
  virtual int f (int x)
    post (r: dd::dchk (r) == 1);	// { dg-warning "contract assertion cannot be proven for a call through 'virtual int B4::f\\(int\\)'" }
};
struct D4 : B4 {
  int f (int x) override	// { dg-bogus "cannot be proven" }
  { return x; }
};

// D5::f's own precondition, whose nested one is provable only through
// B5::f's precondition.
struct B5 { virtual int f (int x) pre (x > 0); };
struct D5 : B5 {
  int f (int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B5::f\\(int\\)'" }
    pre (chk (x) == 1)
  { return x; }
};

// D6::f's own postcondition, unprovable in the override shadow and in
// D6::f's ordinary shadow.
struct B6 { virtual int f (int x); };
struct D6 : B6 {
  int f (int x) override	// { dg-bogus "cannot be proven" }
    post (r: chk (r - 1) == 1)	// { dg-error "contract assertion cannot be proven" }
  { return x; }
};

// D7::f's own precondition with a nested diagnose one, not provable under
// B7::f's precondition either.
struct B7 { virtual int f (int x) pre (x > 0); };
struct D7 : B7 {
  int f (int x) override	// { dg-error "contract assertion cannot be proven for a call through 'virtual int B7::f\\(int\\)'" }
    pre (dd::dchk (x - 1) == 1)	// { dg-warning "contract assertion cannot be proven for a call through 'virtual int B7::f\\(int\\)'" }
  { return x; }
};

void *
make (int i)
{
  switch (i)
    {
    case 1: return new D1;
    case 2: return new D2;
    case 3: return new D3;
    case 4: return new D4;
    case 5: return new D5;
    case 6: return new D6;
    default: return new D7;
    }
}
