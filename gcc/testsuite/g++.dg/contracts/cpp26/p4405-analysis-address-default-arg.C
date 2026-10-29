// D4405 ("Taking the Address of a Function"): a default argument that
// takes the address of a function takes it at each call that uses the
// default argument, and its semantic constraints are checked where it is
// written ([dcl.fct.default]), so that is where a precondition that cannot
// be proven for a call through the address is diagnosed -- an error for
// always_prove, a -Wcontract-unproven warning for always_diagnose -- once,
// whether or not a call uses it, and not again at the calls: by the unary
// &, and by the function-to-pointer conversion or the reference binding
// that converts it to the parameter's type (GCC-927: diagnosed at each call
// using it).  A default argument of a template is diagnosed where it is
// instantiated (for a call that uses it), as everything in a template is;
// one never instantiated is not.  (Its conversion to the parameter's type
// is located at that call, where Clang locates it at the default
// argument.)  A default argument in an unevaluated operand takes no
// address.
//
// Mirror: clang/test/Contracts/p4405-analysis-address-default-arg.cpp in
// the llvm_llvm-project fork.
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

void fp (int x) pre<always_prove> (x > 0);
void fd (int x) pre<always_diagnose> (x > 0);

void a (void (*p) (int) = &fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
void b (void (*p) (int) = fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
void c (void (*p) (int) = fd);	// { dg-warning "contract assertion cannot be proven for a call through the address of 'void fd\\(int\\)'" }
void d (void (&r) (int) = fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
void unused (void (*p) (int) = &fd);	// { dg-warning "contract assertion cannot be proven for a call through the address of 'void fd\\(int\\)'" }
void unused2 (void (*p) (int) = fd);	// { dg-warning "contract assertion cannot be proven for a call through the address of 'void fd\\(int\\)'" }
void e (unsigned long n = sizeof (&fp));

struct S {
  void m (void (*p) (int) = &fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
  void m2 (void (*p) (int) = fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
};

template<class T> void t (void (*p) (int) = &fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
template<class T> void t2 (void (*p) (int) = fp);
template<class T> void never (void (*p) (int) = &fp);
template<class T> void never2 (void (*p) (int) = fp);

template<class T> struct U {
  void m (void (*p) (int) = &fp);	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
};

void
use (S &s, U<int> &u)
{
  a ();
  a ();
  b ();				// { dg-bogus "address" }
  b ();				// { dg-bogus "address" }
  c ();				// { dg-bogus "address" }
  d ();				// { dg-bogus "address" }
  e ();
  s.m ();
  s.m2 ();			// { dg-bogus "address" }
  t<int> ();
  t<int> ();
  t2<int> ();			// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
  t2<int> ();
  u.m ();
  auto l = [] (void (*p) (int) = &fp) {};	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
  l ();
  auto l2 = [] (void (*p) (int) = fp) {};	// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
  l2 ();
}
