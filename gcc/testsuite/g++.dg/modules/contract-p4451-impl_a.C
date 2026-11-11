// P4451: a module interface that imports std without exporting it.  Its
// implementation unit (_b) sees the declaration of
// std::contracts::labels::terminating; an importer (_c) does not, so its
// assertion-control-specifier is rejected.  Under -fcontracts-p4451 alone.

// { dg-additional-options "-fmodules --compile-std-module -fcontracts-p4451" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-module-cmi p4451m }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

export module p4451m;
import std;

export int f (int);
