// P4451: a terminating contract assertion on an exported inline function
// keeps its terminating semantics through the CMI: the importer (_b)
// evaluates it with enforce although ignore is configured.  Under -fcontracts-p4451 alone.

// { dg-additional-options "-fmodules -fcontracts-p4451 -fcontract-evaluation-semantic=ignore" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi p4451s }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

module;
#include <contracts>
export module p4451s;

export inline int fi (int x) pre<terminating> (x > 0) { return x; }
export int fn (int x) pre<terminating> (x > 0) { return x; }
export template<typename T>
T ft (T x) pre<terminating> (x > 0) { return x; }
