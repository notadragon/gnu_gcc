// P4405 stage 3: a diagnostic the proof shadows issue at the end of the
// compilation for an evaluation outside any template instantiation has no
// instantiation backtrace, only its "In function" line.
//
// Mirror: clang/test/Contracts/p4405-shadow-template-context-none.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove" }

int helper (int x)
{
  contract_assert (x > 0);		// { dg-error "contract assertion cannot be proven" }
  return x;
}

int plain (int x)
{
  contract_assert (x > 1);		// { dg-error "contract assertion cannot be proven" }
  return helper (x);
}

// { dg-message "In function 'int helper\\(int\\)'" "" { target *-*-* } 0 }
// { dg-message "In function 'int plain\\(int\\)'" "" { target *-*-* } 0 }
// { dg-bogus "In instantiation|required from" "" { target *-*-* } 0 }
