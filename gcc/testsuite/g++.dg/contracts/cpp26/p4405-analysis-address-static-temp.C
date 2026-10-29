// D4405R0 ("Taking the Address of a Function"; DECISIONS.md E147 (a)): a
// temporary whose lifetime is extended to static or thread storage
// duration is destroyed as an object defined with that storage duration
// is, by a call through its destructor's address, so its creation is
// treated as taking that address (p4405-analysis-address-static-dtor.C has
// the definitions): a precondition of the destructor that allows no
// runtime semantic, and that folding does not prove, cannot be proven for
// that call -- an error under prove (always_prove), a -Wcontract-unproven
// warning under diagnose (always_diagnose).  Diagnosed at the temporary: a
// reference at namespace scope, a static local, thread_local, a static
// data member, a temporary bound to a reference member of a static
// aggregate, a temporary array, the backing array of a static
// std::initializer_list, a template's instantiation (each once).  Not
// diagnosed: an automatic reference (the temporary's destructor call is
// analyzed where it is made), a destructor whose precondition is not
// analyzed, or is tautological (`DC', `pre (true)'), a temporary not
// lifetime-extended, a template never instantiated, a temporary with a
// trivial destructor (a constexpr std::initializer_list of pointers).  GCC
// has no attribute for an object that is never destroyed (Clang's
// [[clang::no_destroy]] case).  (GCC-932: none was diagnosed.)
// -fsyntax-only, as Clang's mirror.
//
// Mirror: clang/test/Contracts/p4405-analysis-address-static-temp.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fsyntax-only" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <initializer_list>

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

int g;

struct DP {
  ~DP () pre<always_prove> (g > 0);	// { dg-message "precondition here" }
};
struct DD {
  ~DD () pre<always_diagnose> (g > 0);	// { dg-message "precondition here" }
};
struct DI {
  ~DI () pre (g > 0);
};
struct DC {
  constexpr ~DC () pre<always_prove> (true) {}	// { dg-bogus "precondition here" }
};

const DP &r1 = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'r1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
DP &&r2 = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'r2', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
const DD &d1 = DD ();	// { dg-warning "contract assertion cannot be proven for a call through the address of 'DD::~DD\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'd1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
const DI &i1 = DI ();
const DC &c1 = DC ();	// { dg-bogus "address" }
thread_local const DP &t1 = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 't1', of thread storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
thread_local const DD &t2 = DD ();	// { dg-warning "contract assertion cannot be proven for a call through the address of 'DD::~DD\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 't2', of thread storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
bool b = (DP (), true);
// A constexpr temporary of a type with no destructor to call.
constexpr std::initializer_list<const char *> cl = {"x", "y"};

struct A {
  const DP &m;
  int n;
};
A a1{DP (), 1};	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'a1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

const DP (&ra)[2] = {DP (), DP ()};	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'ra', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
std::initializer_list<DP> il = {DP ()};	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'il', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

struct S {
  static const DP &s1;
  static inline const DP &s2 = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'S::s2', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
};
const DP &S::s1 = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'S::s1', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }

void
f ()
{
  static const DP &sl = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'sl', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
  const DI &al = DI ();
  (void) al;
  static const DI &il2 = DI ();
  (void) il2;
}

template <class T>
void
tf ()
{
  static const DP &st = DP ();	// { dg-error "contract assertion cannot be proven for a call through the address of 'DP::~DP\\(\\)'" }
// { dg-message "a temporary lifetime-extended by 'st', of static storage duration, is destroyed by a call through the address of its destructor" "" { target *-*-* } .-1 }
}
template <class T> void never () { static const DP &sn = DP (); }

void
use ()
{
  tf<int> ();
  tf<char> ();
}
