// The refinable mark is streamed, and inlining a refinable body at link
// time marks the function it is inlined into.
// { dg-lto-do link }
// { dg-lto-options { { -O2 -fno-early-inlining -flto -flto-partition=one -fdump-ipa-inline --param=ipa-refinable-inline=1 } } }

#include "refinable-2.h"

int mid (int);

int
main ()
{
  return mid (leaf (0));
}

// LEAF is marked in both units; MID, in the other unit, is not declared
// inline and gets marked when LEAF is inlined into it.
// { dg-final { scan-wpa-ipa-dump "_Z3midi/\[0-9\]+ \\(mid\\)\n(  \[^\n\]*\n){0,5}  Availability: \[a-z\]+ \\(refinable body\\)" "inline" } }
