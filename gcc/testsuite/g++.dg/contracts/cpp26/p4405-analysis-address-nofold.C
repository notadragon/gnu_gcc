// D4405 ("Taking the Address of a Function"): only folding can prove a
// precondition for a call through the address of its function, so with
// -fno-contracts-prove-fold even a tautological precondition that allows no
// runtime semantic is diagnosed where the address is taken
// (p4405-analysis-address-tautological.C has it with folding).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fno-contracts-prove-fold" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

void t1 (int x) pre<always_prove> (true);	// { dg-message "precondition here" }
void t2 (int x) pre<always_prove> (sizeof (x) == sizeof (int));	// { dg-message "precondition here" }

struct DT {
  ~DT () pre<always_prove> (true);	// { dg-message "precondition here" }
};

DT dt1;						// { dg-error "contract assertion cannot be proven for a call through the address of 'DT::~DT\\(\\)'" }
// { dg-message "'dt1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

void (*p1) (int) = t1;				// { dg-error "contract assertion cannot be proven for a call through the address of 'void t1\\(int\\)'" }
void (*p2) (int) = &t2;				// { dg-error "contract assertion cannot be proven for a call through the address of 'void t2\\(int\\)'" }
