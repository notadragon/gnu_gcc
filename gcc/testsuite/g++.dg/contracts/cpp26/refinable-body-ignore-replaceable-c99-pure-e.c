/* Extra translation unit of refinable-body-ignore-replaceable-c99-pure.C,
   compiled as C: the external definition of f, its assertion observed.  */

#include <contracts.h>

inline int f (int x)
{
  _ContractAssert (x > 0);	/* observe */
  return x;
}

extern int f (int);
