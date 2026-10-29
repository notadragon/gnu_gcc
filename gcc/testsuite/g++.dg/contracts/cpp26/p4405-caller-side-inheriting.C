// P4405: a call of an inheriting constructor evaluates the inherited
// constructor's caller-side evaluations, so their prove and diagnose are
// diagnosed at each call, folded with its arguments, and never at the
// using-declaration: through two levels of inheriting constructors, from a
// virtual base, from a class template's specialization, from a constructor
// template (declared only; one with a definition is in
// p4405-fold-caller-side-template.C), in a new-expression, and not in an
// unevaluated operand.  -fmax-errors=9 makes a repeated error fail the
// test: a tenth error would end the compilation before the last expected
// one.
//
// Mirror: clang/test/Contracts/p4405-caller-side-inheriting.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fmax-errors=9" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-fold-caller-side.json" }

struct S {
  S (int x) pre (x > 0);	// { dg-message "the predicate of the contract assertion" }
};
struct T : S {
  using S::S;			// { dg-bogus "cannot be proven|always false" }
};
struct T2 : T {
  using T::T;			// { dg-bogus "cannot be proven|always false" }
};
struct TV : virtual S {
  using S::S;			// { dg-bogus "cannot be proven|always false" }
};

template <class X>
struct SX {
  SX (X x) pre (x > 0);		// { dg-message "the predicate of the contract assertion" }
};
struct TX : SX<int> {
  using SX<int>::SX;		// { dg-bogus "cannot be proven|always false" }
};

struct ST {
  template <class U>
  ST (U u) pre (u > 0);		// { dg-message "the predicate of the contract assertion" }
};
struct TT : ST {
  using ST::ST;			// { dg-bogus "cannot be proven|always false" }
};

void use (int n)
{
  T a (n);			// { dg-error "contract assertion cannot be proven" }
  T b (n);			// { dg-error "contract assertion cannot be proven" }
  T c (2);			// proven
  T2 d (n);			// { dg-error "contract assertion cannot be proven" }
  T2 e (0);			// { dg-error "contract predicate is always false" }
  T2 f (3);			// proven
  TV g (n);			// { dg-error "contract assertion cannot be proven" }
  TV h (1);			// proven
  TX i (0);			// { dg-error "contract predicate is always false" }
  TX j (4);			// proven
  TT k (n);			// { dg-error "contract assertion cannot be proven" }
  TT l (0);			// { dg-error "contract predicate is always false" }
  TT m (5);			// proven
  T *p = new T (0);		// { dg-error "contract predicate is always false" }
  decltype (T (0)) *q = p;	// { dg-bogus "always false" }
}
