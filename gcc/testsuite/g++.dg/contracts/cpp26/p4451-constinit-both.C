// P4451: whether a variable is constant-initialized is decided with every
// contract assertion using the ignore semantic ([expr.const.init]), so the
// initializer below is a constant initializer; its evaluation then uses a
// terminating semantic, and the false predicate makes it fail.  Under -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

constexpr int l (int x) pre<terminating> (x != 1) { return x; }	// { dg-error "contract predicate is false" }
const int i1 = l (1);
