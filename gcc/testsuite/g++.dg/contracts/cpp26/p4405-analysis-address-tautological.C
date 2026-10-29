// D4405 ("Taking the Address of a Function"): a precondition that allows no
// runtime semantic is diagnosed where the address of its function is taken
// only when folding, with nothing known of the arguments, does not prove it
// true -- a tautological precondition (`true', `sizeof (x) == sizeof (int)',
// a constant expression) is not diagnosed, under prove or diagnose, by any
// form of taking the address: the unary &, the function-to-pointer
// conversion, a reference, a pointer to member function, a lambda's
// conversion, the definition of an object of static storage duration (its
// destructor) and a lifetime-extended temporary.  One that folds to false
// is "always false" for a call through the address.  With several
// preconditions, the first that is not proven is reported, an error before
// a warning.  With -fno-contracts-prove-fold nothing is proven, so the
// tautological ones are diagnosed (p4405-analysis-address-nofold.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
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

constexpr int k = 3;
int g;

void t1 (int x) pre<always_prove> (true);
void t2 (int x) pre<always_prove> (sizeof (x) == sizeof (int));
void t3 (int x) pre<always_prove> (k > 0);
void t4 (int x) pre<always_diagnose> (sizeof (x) == sizeof (int));
void t5 (int x) pre<always_prove> (true) pre<always_diagnose> (k == 3);
void z1 (int x) pre<always_prove> (false);	// { dg-message "precondition here" }
void z2 (int x) pre<always_diagnose> (k < 0);	// { dg-message "precondition here" }
void m1 (int x) pre<always_prove> (true) pre<always_diagnose> (x > 0) pre<always_prove> (x < 9);	// { dg-message "precondition here" }
void m2 (int x) pre<always_prove> (true) pre<always_diagnose> (x > 0);	// { dg-message "precondition here" }

struct S {
  void mt (int x) pre<always_prove> (sizeof (x) == 4 || sizeof (x) != 4);
  static void st (int x) pre<always_prove> (true);
};

template<typename T>
void tt (T x) pre<always_prove> (sizeof (T) == sizeof (x));

struct DT {
  ~DT () pre<always_prove> (true);
};
struct DK {
  ~DK () pre<always_diagnose> (k == 3);
};
struct DZ {
  ~DZ () pre<always_prove> (k == 4);	// { dg-message "precondition here" }
};

DT dt1;						// { dg-bogus "address" }
DK dk1;						// { dg-bogus "address" }
thread_local DT dt2;				// { dg-bogus "address" }
DT dta[2];					// { dg-bogus "address" }
const DT &rt = DT ();				// { dg-bogus "address" }
DZ dz1;						// { dg-error "contract predicate is always false for a call through the address of 'DZ::~DZ\\(\\)'" }
// { dg-message "'dz1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

void
use ()
{
  void (*p1) (int) = t1;			// { dg-bogus "address" }
  void (*p2) (int) = &t2;			// { dg-bogus "address" }
  void (&r3) (int) = t3;			// { dg-bogus "address" }
  void (*p4) (int) = t4;			// { dg-bogus "address" }
  void (*p5) (int) = t5;			// { dg-bogus "address" }
  auto pm = &S::mt;				// { dg-bogus "address" }
  void (*ps) (int) = S::st;			// { dg-bogus "address" }
  void (*pt) (int) = tt<int>;			// { dg-bogus "address" }
  auto l = [] (int x) pre<always_prove> (sizeof (x) > 0) { return x; };
  int (*pl) (int) = l;				// { dg-bogus "address" }
  static DT dt3;				// { dg-bogus "address" }
  void (*q1) (int) = z1;			// { dg-error "contract predicate is always false for a call through the address of 'void z1\\(int\\)'" }
  void (*q2) (int) = z2;			// { dg-warning "contract predicate is always false for a call through the address of 'void z2\\(int\\)' \\\[-Wcontract-unproven\\\]" }
  void (*q3) (int) = m1;			// { dg-error "contract assertion cannot be proven for a call through the address of 'void m1\\(int\\)'" }
  void (*q4) (int) = m2;			// { dg-warning "contract assertion cannot be proven for a call through the address of 'void m2\\(int\\)'" }
  (void) p1; (void) p2; (void) r3; (void) p4; (void) p5; (void) pm;
  (void) ps; (void) pt; (void) pl; (void) q1; (void) q2; (void) q3;
  (void) q4;
}
