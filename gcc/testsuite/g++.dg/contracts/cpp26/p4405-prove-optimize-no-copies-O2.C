// D4405's proof optimization level (DECISIONS.md E162-E168, E206): by
// default the proof level of a -O2 build is the build's own level, so the
// callees the shadows reach are at the proof level already and no level
// copy is made; the premise copies are optimized at that level too.  The
// example is D4405R0's Peter Bindels `expected` example
// (p4405-shadow-callee-premises-bindels.C) with observe.
//
// Mirror: none (GCC's level copies).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=observe -fdump-ipa-contract_shadow_clone" }

#include "p4405-shadow-callee-premises-bindels-observe.C"

// { dg-final { scan-ipa-dump "Premise copy" "contract_shadow_clone" } }
// { dg-final { scan-ipa-dump-not "Level copy" "contract_shadow_clone" } }
