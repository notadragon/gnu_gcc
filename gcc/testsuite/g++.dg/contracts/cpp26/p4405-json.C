// P4405: prove and diagnose selected by a contract configuration, per kind.
// D4405: a precondition's is diagnosed at each call.
//
// Mirror: clang/test/Contracts/p4405-json.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-json.json" }

int f (int x)
  pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  post (r: r > 0)		// { dg-warning "contract assertion cannot be proven" }
{
  contract_assert (x != 3);	// { dg-warning "contract assertion cannot be proven" }
  return x;
}

int g (int y)
{
  return f (y);			// { dg-error "contract assertion cannot be proven" }
}
