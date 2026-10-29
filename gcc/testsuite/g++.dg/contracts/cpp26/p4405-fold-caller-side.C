// P4405 stage 2: a caller-side evaluation of a precondition is folded with
// the call's arguments substituted for the parameters, when the argument
// folds to a constant and the parameter is passed by value and has scalar
// type.  Every other case -- a reference parameter, a non-constant
// argument -- keeps the parameter unknown, and so stays unproven unless the
// predicate holds whatever its value.  The callee-side
// evaluations are ignore, so the definitions are not diagnosed.
//
// Mirror: clang/test/Contracts/p4405-fold-caller-side.cpp in the
// llvm_llvm-project fork.  One call differs: Clang does not fold d == d for
// an unknown d, so either (n, m) is unproven there.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-fold-caller-side.json" }

int divide (int n, int d) pre (d != 0);	// { dg-message "the predicate of the contract assertion" }
int scale (int x, int f = 0) pre (f != 0);	// { dg-message "the predicate of the contract assertion" }
int scale3 (int x, int f = 3) pre (f > 0);
int byref (const int &d) pre (d != 0);	// { dg-message "the predicate of the contract assertion" }
int deref (int *p) pre (p != nullptr);	// { dg-message "the predicate of the contract assertion" }
int va (int n, ...) pre (n > 0);
int either (int n, int d) pre (d == d);
enum class E { a, b };
int pick (E e) pre (e == E::b);		// { dg-message "the predicate of the contract assertion" }
int k ();
int global;
constexpr int two = 2;

struct S {
  S (int x) pre (x > 0);		// { dg-message "the predicate of the contract assertion" }
  void m (int x) pre (x > 0);		// { dg-message "the predicate of the contract assertion" }
  void self (const S *p) pre (p == this);	// { dg-message "the predicate of the contract assertion" }
};
S gs (1);				// { dg-bogus "cannot be proven|always false" }

// A call of an inheriting constructor is folded with its arguments for the
// inherited constructor's parameters.
struct T : S {
  using S::S;				// { dg-bogus "cannot be proven|always false" }
};

// A virtual base gives the constructor an in-charge parameter, which the
// complete object constructor the call calls has not: the arguments are
// substituted all the same (GCC-917).
struct V {};
struct W : virtual V {
  W (int x) pre (x > 0);		// { dg-message "the predicate of the contract assertion" }
};

template <class... T>
void all_positive (T... t) pre (((t > 0) && ...));	// { dg-message "the predicate of the contract assertion" }

void use (int n, int m, S *ps)
{
  divide (n, 0);		// { dg-error "contract predicate is always false" }
  divide (n, 2);		// proven
  divide (n, two);		// proven
  divide (n, 1 + 1);		// proven
  divide (n, m);		// { dg-error "contract assertion cannot be proven" }
  divide (n, k ());		// { dg-error "contract assertion cannot be proven" }
  divide (n, (k (), 2));	// { dg-error "contract assertion cannot be proven" }
  either (n, m);		// proven whatever m is
  scale (n);			// { dg-error "contract predicate is always false" }
  scale3 (n);			// proven
  byref (0);			// { dg-error "contract assertion cannot be proven" }
  deref (nullptr);		// { dg-error "contract predicate is always false" }
  deref (&global);		// proven
  va (1, 2, 3);			// proven
  pick (E::b);			// proven
  pick (E::a);			// { dg-error "contract predicate is always false" }
  S s (1);			// proven
  S t (0);			// { dg-error "contract predicate is always false" }
  s.m (2);			// proven
  T u (1);			// proven: the inherited constructor's arguments
  T v (0);			// { dg-error "contract predicate is always false" }
  s.m (0);			// { dg-error "contract predicate is always false" }
  ps->m (3);			// proven
  gs.self (&gs);		// proven: `this' is the static object gs
  ps->self (&gs);		// { dg-error "contract assertion cannot be proven" }
  W w (1);			// proven
  W w0 (0);			// { dg-error "contract predicate is always false" }
  all_positive (1, 2);		// proven
  all_positive (1, 0);		// { dg-error "contract predicate is always false" }
  decltype (divide (n, 0)) z = 0;	// { dg-bogus "always false" }
}
