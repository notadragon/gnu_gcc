// D4405: -Wno-contract-unproven silences the warning for taking the
// address of a function whose precondition allows only diagnose
// (always_diagnose), but not the error for one that allows only prove
// (always_prove; p4405-analysis-address.C).  A precondition that only the
// configuration analyzes is not diagnosed at all
// (p4405-analysis-address-flag.C).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove -Wno-contract-unproven" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int f (int x) pre (x > 0);
int fd (int x) pre<always_diagnose> (x > 0);	// { dg-bogus "precondition here" }
int fp (int x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }

int (*p) (int) = f;				// { dg-bogus "address" }
int (*d) (int) = fd;				// { dg-bogus "address" }
int (*q) (int) = fp;				// { dg-error "contract assertion cannot be proven for a call through the address of 'int fp\\(int\\)'" }
