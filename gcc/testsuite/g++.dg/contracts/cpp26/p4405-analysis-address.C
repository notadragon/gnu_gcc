// D4405 ("Taking the Address of a Function"): a call through a pointer or
// a reference to a function cannot identify the function, so it has no
// analysis evaluation and a precondition can be proven for it only if it is
// tautological.  Taking the address of a function -- the unary &, the
// function-to-pointer conversion, forming a pointer to member function,
// binding a reference -- is diagnosed when a precondition of it allows no
// runtime semantic and folding does not prove it: "contract assertion
// cannot be proven for a call through the address of", an error under
// prove (always_prove) and a -Wcontract-unproven warning under diagnose
// (always_diagnose).  A precondition that is not analyzed draws nothing,
// nor does an unevaluated operand, nor a virtual call (P3097), which
// analyzes the precondition of the function it names.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405" }
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

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void fp (int x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }
void fd (int x) pre<always_diagnose> (x > 0);	// { dg-message "precondition here" }
void fn (int x) pre (x > 0);
void fo (int x);
void fo (double x) pre<always_prove> (x > 0);

struct S {
  void mp (int x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }
  static void sd (int x) pre<always_diagnose> (x > 0);	// { dg-message "precondition here" }
  virtual void vd (int x) pre<always_diagnose> (x > 0);	// { dg-message "precondition here" }
};

template<typename T>
void tp (T x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }

void (*gp) (int) = fp;				// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }

void use ()
{
  void (*p1) (int) = fp;			// { dg-error "cannot be proven for a call through the address" }
  void (*p2) (int) = &fp;			// { dg-error "cannot be proven for a call through the address" }
  void (&r1) (int) = fp;			// { dg-error "cannot be proven for a call through the address" }
  void (*p3) (int) = fd;			// { dg-warning "contract assertion cannot be proven for a call through the address of 'void fd\\(int\\)' \\\[-Wcontract-unproven\\\]" }
  void (*p4) (int) = fn;			// { dg-bogus "address" }
  void (*p5) (int) = fo;			// { dg-bogus "address" }
  void (*p6) (double) = fo;			// { dg-error "cannot be proven for a call through the address" }
  auto pm = &S::mp;				// { dg-error "cannot be proven for a call through the address" }
  void (*ps) (int) = &S::sd;			// { dg-warning "cannot be proven for a call through the address" }
  auto pv = &S::vd;				// { dg-warning "cannot be proven for a call through the address" }
  void (*pt) (int) = tp<int>;			// { dg-error "cannot be proven for a call through the address" }
  decltype (&fp) q = nullptr;			// { dg-bogus "address" }
  (void) sizeof (&fp);				// { dg-bogus "address" }
  (void) p1; (void) p2; (void) r1; (void) p3; (void) p4; (void) p5;
  (void) p6; (void) pm; (void) ps; (void) pv; (void) pt; (void) q;
}

void call (S &s)
{
  s.vd (1);					// { dg-bogus "address|cannot be proven" }
  fp (1);					// { dg-bogus "address|cannot be proven" }
}
