// D4405 ("The Analysis Evaluation"): p4405-analysis-elided-temporary-
// aggregate.C with optimization, where the proof shadow decides what folding
// does not, and with `diagnose', so that no error stops the middle end.  The
// cleanup of an aggregate's already-built member is analyzed whether or not
// anything after the member can throw (DECISIONS.md E160 (a)): when nothing
// can, the shadow used to remove that EH-only cleanup, and with it the
// marker, as unreachable, which proved the evaluation (GCC-949) in a
// function and for a static local.  Code generation, not -fsyntax-only.
// Mirror: llvm_llvm-project
// clang/test/Contracts/p4405-analysis-elided-temporary-aggregate.cpp (-O2).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int g;

struct DP {
  DP ();
  ~DP () pre<always_diagnose> (g > 0);	// { dg-message "predicate of the contract assertion" }
};
DP make ();
int thr ();
struct A { DP p; int i; };	// { dg-warning "contract assertion cannot be proven" }

// Namespace scope.
A na{DP (), thr ()};	// { dg-warning "contract assertion cannot be proven" }
A nb{make (), thr ()};	// { dg-warning "contract assertion cannot be proven" }
struct SA { static A sa; };
A SA::sa{DP (), thr ()};	// { dg-warning "contract assertion cannot be proven" }

// In a function.
void
f ()
{
  A b{DP (), thr ()};	// { dg-warning "contract assertion cannot be proven" }
  static A c{DP (), thr ()};	// { dg-warning "contract assertion cannot be proven" }
  (void) b;
  (void) c;
}

// DECISIONS.md E160 (a): the member's destructor is potentially invoked
// ([dcl.init.aggr]) until the initialization completes, even when nothing
// after it can throw (`A e1{DP (), 1};', a `noexcept' call after it): the
// cleanup GCC builds is analyzed all the same -- for a base, a member
// initialized by parentheses, a default member initializer, a lambda's
// capture and an array's element too.  A later initialization is one the
// initializer or a default member initializer gives; a member left to
// implicit value-initialization (`A e6{DP ()};') is not one, and the last
// member (`L e7{1, DP ()};') has nothing after it.  The variables of static
// storage duration of a type whose destructor has the precondition get the
// address-of error besides.  Mirror: llvm_llvm-project
// clang/test/Contracts/p4405-analysis-elided-temporary-aggregate.cpp.
int nothr () noexcept;
struct DN {
  DN () noexcept;
  ~DN () pre<always_diagnose> (g > 0);	// { dg-message "predicate of the contract assertion" }
  // { dg-message "precondition here" "" { target *-*-* } .-1 }
};
struct AI { DP p; int i = 5; };	// { dg-warning "contract assertion cannot be proven" }
struct L { int i; DP p; };	// { dg-warning "contract assertion cannot be proven" }
// D declares its destructor (an aggregate may), defined in no translation
// unit here, so only its base's partial cleanup is analyzed.
struct D : DP { int i; ~D (); };
// An array member's elements are destroyed in a loop.
struct AR { DN a[2]; int i; };	// { dg-warning "contract assertion cannot be proven" }
// A nested aggregate: j follows a.p, so a.p's cleanup is potentially
// invoked; with nothing after a, it is not.
struct O { A a; int j; };

A e1{DP (), 1};			// { dg-warning "contract assertion cannot be proven" }
A e2{DP (), nothr ()};		// { dg-warning "contract assertion cannot be proven" }
A e3(DP (), 1);			// { dg-warning "contract assertion cannot be proven" }
D e4{{}, 1};			// { dg-warning "contract assertion cannot be proven" }
AI e5{DP ()};			// { dg-warning "contract assertion cannot be proven" }
A e6{DP ()};			// { dg-bogus "cannot be proven" }
L e7{1, DP ()};			// { dg-bogus "cannot be proven" }
struct SE { static A se; };
A SE::se{DP (), 1};		// { dg-warning "contract assertion cannot be proven" }
DN e8[2] = {DN (), DN ()};	// { dg-warning "contract assertion cannot be proven" }
// { dg-message "of static storage duration" "" { target *-*-* } .-1 }
O e9{{DP ()}, 1};		// { dg-warning "contract assertion cannot be proven" }
O e10{{DP ()}};			// { dg-bogus "cannot be proven" }

void
h ()
{
  A b1{DP (), 1};		// { dg-warning "contract assertion cannot be proven" }
  static A b2{DP (), nothr ()};	// { dg-warning "contract assertion cannot be proven" }
  A b3(DP (), 1);		// { dg-warning "contract assertion cannot be proven" }
  D b4{{}, 1};			// { dg-warning "contract assertion cannot be proven" }
  static AI b5{DP ()};		// { dg-warning "contract assertion cannot be proven" }
  A b6{DP ()};			// { dg-bogus "cannot be proven" }
  L b7{1, DP ()};		// { dg-bogus "cannot be proven" }
  // The closure's destructor, and the capture's partial cleanup.
  auto b8 = [p = DP (), i = 1]	// { dg-warning "contract assertion cannot be proven" }
    {};				// { dg-warning "contract assertion cannot be proven" }
  static DN b9[2] = {DN (), DN ()};	// { dg-warning "contract assertion cannot be proven" }
  // { dg-message "of static storage duration" "" { target *-*-* } .-1 }
  AR b10{{DN (), DN ()}, 1};		// { dg-warning "contract assertion cannot be proven" }
  // A capture that comes last: only the closure's destructor, no partial
  // cleanup of p.
  auto b11 = [i = 1, p = DP ()]	// { dg-warning "contract assertion cannot be proven" }
    {};				// { dg-bogus "cannot be proven" }
  // An array member with no later member: a[1] follows a[0], so a[0]'s
  // cleanup is still potentially invoked.
  AR b12{{DN (), DN ()}};		// { dg-warning "contract assertion cannot be proven" }
  (void) b1;
  (void) b2;
  (void) b3;
  (void) b4;
  (void) b5;
  (void) b6;
  (void) b7;
  (void) b8;
  (void) b10;
  (void) b11;
  (void) b12;
}
