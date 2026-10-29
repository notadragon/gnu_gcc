// DECISIONS.md E148 (b): an array of static or thread storage duration is
// destroyed at exit by a function the implementation generates (__tcf_*),
// which calls the destructor for each element directly, and those calls
// are analyzed like any other.  So besides the address-of diagnostic its
// definition may get (D4405R0 "Taking the Address of a Function",
// p4405-analysis-address-static-dtor.C, and for a temporary
// p4405-analysis-address-static-temp.C), code generation reports the
// destructor precondition's analysis evaluation: "cannot be proven", an
// error under prove and a -Wcontract-unproven warning under diagnose.  A
// non-array object has no such call.  GCC also reports the destructor
// calls of the initializer's cleanup (build_vec_init's partial-array
// destruction, and for a temporary array each element temporary's), at the
// same line -- at the declarator, its initializer, or an element; Clang
// reports the generated function's calls only.
//
// The prove case here is a precondition analyzed with prove through
// -fcontract-analysis-semantic=prove, whose runtime semantic stays
// enforce, so the definition gets no address-of diagnostic -- only the
// configuration selects prove -- and only the generated calls are
// reported; always_diagnose (`DD') gets the address-of warning as well.
// p4405-analysis-static-array-dtor.C is the same at -O0.  (GCC-933.)
//
// Mirror: clang/test/Contracts/p4405-analysis-static-array-dtor.cpp in the
// llvm_llvm-project fork (its two RUN lines, -O0 and -O2).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <initializer_list>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

int g;

struct DI {
  ~DI () pre (g > 0);	// { dg-message "the predicate of the contract assertion" }
// { dg-bogus "precondition here" "" { target *-*-* } .-1 }
};
struct DD {
  ~DD () pre<always_diagnose> (g > 0);	// { dg-message "precondition here" }
// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
};

DI ia[2];	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
DD da[2];	// { dg-warning "contract assertion cannot be proven for a call through the address of 'DD::~DD\\(\\)'" }
// { dg-message "'da', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
// { dg-warning "contract assertion cannot be proven" "" { target *-*-* } .-2 }
thread_local DI ta[2];	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
DI ma[2][3];	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }

// Not an array, and only the configuration selects prove: nothing.
DI i1;	// { dg-bogus "address" }
// { dg-bogus "contract assertion cannot be proven" "" { target *-*-* } .-1 }

struct S {
  static DI sa[2];
};
DI S::sa[2];	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }

void
f ()
{
  static DI sl[2];	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
  static DD sd[3];	// { dg-warning "contract assertion cannot be proven for a call through the address of 'DD::~DD\\(\\)'" }
// { dg-message "'sd', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
// { dg-warning "contract assertion cannot be proven" "" { target *-*-* } .-2 }
}

// Temporaries of static storage duration (E147 (a)): an array, and the
// backing array of a std::initializer_list.
const DI (&ra)[2] = {DI (), DI ()};	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
std::initializer_list<DI> il = {DI (), DI ()};	// { dg-bogus "address" }
// { dg-error "contract assertion cannot be proven" "" { target *-*-* } .-1 }
