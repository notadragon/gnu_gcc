/* Extra translation unit of refinable-body-ignore-replaceable-c99.C, compiled
   as C: the external definition of f (the same inline definition, made
   external by the extern declaration), its assertion enforced.  */

#include <contracts.h>

inline int f (int x)
{
  _ContractAssert (x > 0);	/* enforce */
  return x;
}

extern int f (int);
