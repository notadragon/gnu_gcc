// P4405 stage 2: a caller-side evaluation of a precondition of a
// specialization of a template with a definition is folded with the call's
// arguments, as for any other function.  The specialization's contracts are
// substituted only when its definition is instantiated, after the call, so
// the predicate is substituted privately for the proof: a function
// template, a constructor template (directly and through an inheriting
// constructor), a member template, a member function and a constructor of a
// class template (through an inheriting constructor too), a member template
// of a class template, a pack, default template arguments, a non-type
// template parameter, and a P4283 requires-clause, which is honoured: a
// precondition it discards is not evaluated, so not diagnosed.  A dependent
// predicate the arguments do not decide stays unproven, and one holding a
// lambda is left unsubstituted, and stays unproven.  The callee-side
// evaluations are ignore, so the definitions are not diagnosed.
//
// Mirror: clang/test/Contracts/p4405-template-caller.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-p4283" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-fold-caller-side.json" }

template <class U>
void hd (U u) pre (u > 0) {}	// { dg-message "the predicate of the contract assertion" }

struct SD {
  template <class U>
  SD (U u) pre (u > 0) {}	// { dg-message "the predicate of the contract assertion" }
};
struct TD : SD {
  using SD::SD;			// { dg-bogus "cannot be proven|always false" }
};

struct M {
  template <class U>
  void f (U u) pre (u > 0) {}	// { dg-message "the predicate of the contract assertion" }
};

template <class T>
struct CT {
  CT (T t) pre (t > 0) {}	// { dg-message "the predicate of the contract assertion" }
  void g (T t) pre (t != 0) {}	// { dg-message "the predicate of the contract assertion" }
  template <class U>
  void h (T t, U u) pre (t < u) {}	// { dg-message "the predicate of the contract assertion" }
};
struct CD : CT<int> {
  using CT<int>::CT;		// { dg-bogus "cannot be proven|always false" }
};

template <class... T>
void allp (T... t) pre (((t > 0) && ...)) {}	// { dg-message "the predicate of the contract assertion" }

template <class U = int, int N = 3>
void dflt (U u = N) pre (u > 0) {}	// { dg-message "the predicate of the contract assertion" }

template <int N>
void below (int x) pre (x < N) {}	// { dg-message "the predicate of the contract assertion" }

template <class U>
void big (U) pre (sizeof (U) > 64) {}	// { dg-message "the predicate of the contract assertion" }

struct Val { int val; };
template <class T>
concept HasVal = requires (T t) { t.val; };
template <class T>
void rq (T x) pre requires (HasVal<T>) (x.val > 0) {}	// { dg-message "the predicate of the contract assertion" }

template <class U>
void lam (U u) pre ([] (U v) { return v > 0; } (u)) {}	// { dg-message "the predicate of the contract assertion" }

template <class U>
void cls (U u) pre (u.val > 0) {}	// { dg-message "the predicate of the contract assertion" }

void use (int n, M *pm, CT<int> *pc)
{
  hd (1);			// proven
  hd (0);			// { dg-error "contract predicate is always false" }
  hd (2.5);			// proven
  hd (n);			// { dg-error "contract assertion cannot be proven" }
  SD a (1);			// proven
  SD b (0);			// { dg-error "contract predicate is always false" }
  TD c (1);			// proven: the inherited constructor's arguments
  TD d (0);			// { dg-error "contract predicate is always false" }
  pm->f (1);			// proven
  pm->f (0L);			// { dg-error "contract predicate is always false" }
  CT<int> e (1);		// proven
  CT<int> f (0);		// { dg-error "contract predicate is always false" }
  CD g (1);			// proven
  CD h (0);			// { dg-error "contract predicate is always false" }
  pc->g (1);			// proven
  pc->g (0);			// { dg-error "contract predicate is always false" }
  pc->h (1, 2L);		// proven
  pc->h (2, 1L);		// { dg-error "contract predicate is always false" }
  pc->h (n, 1L);		// { dg-error "contract assertion cannot be proven" }
  allp (1, 2L, 3u);		// proven
  allp (1, 0, 3);		// { dg-error "contract predicate is always false" }
  allp ();			// proven: an empty && fold is true
  dflt ();			// proven: u is N, 3
  dflt<int, 0> ();		// { dg-error "contract predicate is always false" }
  dflt (n);			// { dg-error "contract assertion cannot be proven" }
  below<5> (3);			// proven
  below<5> (7);			// { dg-error "contract predicate is always false" }
  big (n);			// { dg-error "contract predicate is always false" }
  rq (n);			// discarded by its requires-clause: not evaluated
  rq (Val {1});			// { dg-error "contract assertion cannot be proven" }
  lam (1);			// { dg-error "contract assertion cannot be proven" }
  cls (Val {1});		// { dg-error "contract assertion cannot be proven" }
}
