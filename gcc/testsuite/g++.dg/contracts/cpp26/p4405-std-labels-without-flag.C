// D4405: without -fcontracts-p4405, <contracts> declares none of the
// standard labels prove, diagnose, always_prove and always_diagnose, while
// the P3400 labels are there (p4405-std-labels.C: with the flag).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace L = std::contracts::labels;

#ifdef __cpp_contracts_prove
#error "__cpp_contracts_prove defined without -fcontracts-p4405"
#endif

constexpr auto ok = L::always_enforce;

constexpr auto a = L::prove;		// { dg-error "'prove' is not a member of 'L'" }
constexpr auto b = L::diagnose;		// { dg-error "'diagnose' is not a member of 'L'" }
constexpr auto c = L::always_prove;	// { dg-error "'always_prove' is not a member of 'L'" }
constexpr auto d = L::always_diagnose;	// { dg-error "'always_diagnose' is not a member of 'L'" }
L::prove_t e;				// { dg-error "'prove_t' in namespace 'L' does not name a type" }
L::always_diagnose_t f;			// { dg-error "'always_diagnose_t' in namespace 'L' does not name a type" }

void g (int x) pre<prove> (x > 0);	// { dg-error "'prove' was not declared in this scope" }
