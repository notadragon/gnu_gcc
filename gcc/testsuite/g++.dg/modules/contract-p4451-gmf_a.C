// P4451: <contracts> included in a module's global module fragment does
// not make std::contracts::labels::terminating visible to an importer
// (_b).  Under -fcontracts-p4451 alone.

// { dg-additional-options "-fmodules -fcontracts-p4451" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi p4451g }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

module;
#include <contracts>
export module p4451g;

export int x;
