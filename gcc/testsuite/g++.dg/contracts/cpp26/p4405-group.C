// P4405: prove and diagnose selected by -fcontract-group-evaluation-semantic=
// for the groups of an assertion-control label; a contract in neither group
// keeps the command-line semantic.
//
// Mirror: clang/test/Contracts/p4405-group.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-group-evaluation-semantic=g:prove,h:diagnose -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int f (int x) pre<"g"group> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int g (int x) pre<"h"group> (x > 0) { return x; }	// { dg-message "the predicate of the contract assertion" }
int h (int x) pre (x > 0) { return x; }			// { dg-bogus "cannot be proven" }

int use (int y)
{
  return f (y)		// { dg-error "contract assertion cannot be proven" }
	 + g (y)	// { dg-warning "contract assertion cannot be proven" }
	 + h (y);	// { dg-bogus "cannot be proven" }
}
