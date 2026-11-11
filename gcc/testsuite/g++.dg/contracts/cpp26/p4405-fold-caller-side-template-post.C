// P4405 stage 2: a caller-side evaluation of a postcondition of a
// specialization of a template with a definition is folded with the
// predicate substituted, as p4405-fold-caller-side-template.C does for a
// precondition: a predicate that the template arguments decide is proven
// or always false.  One holding a lambda, and a dependent one of a
// postcondition with captures, are not substituted, and stay unproven.
// The callee-side evaluations are ignore, so the definitions are not
// diagnosed.
//
// Mirror: clang/test/Contracts/p4405-template-caller-post.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontracts-p3098" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-fold-caller-side-template-post.json" }

template <class U>
U lit (U u) post (true) { return u; }

template <class U>
U small (U u) post (sizeof (U) <= 8) { return u; }	// { dg-message "the predicate of the contract assertion" }

template <class U>
U res (U u) post (r: r > 0) { return u; }	// { dg-message "the predicate of the contract assertion" }

template <class U>
U lam (U u) post (r: [] (U v) { return v > 0; } (r)) { return u; }	// { dg-message "the predicate of the contract assertion" }

template <class U>
U cap (U u) post [s = sizeof (U)] (s <= 8) { return u; }	// { dg-message "the predicate of the contract assertion" }

struct Big { char c[100]; };

void use (int n)
{
  lit (n);			// proven
  small (n);			// proven
  small (Big {});		// { dg-error "contract predicate is always false" }
  res (1);			// { dg-error "contract assertion cannot be proven" }
  lam (1);			// { dg-error "contract assertion cannot be proven" }
  cap (n);			// { dg-error "contract assertion cannot be proven" }
}
