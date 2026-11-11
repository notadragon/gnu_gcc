// P4451: <contracts> included in a module's global module fragment does
// not make std::contracts::labels::terminating visible to an importer
// (_b).  Under -fcontracts-p3400 alone (the P3400 label).

// { dg-additional-options "-fmodules -fcontracts-p3400" }
// { dg-do compile { target c++26 } }
// { dg-module-cmi p4451g_p3400 }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

module;
#include <contracts>
export module p4451g_p3400;

export int x;
