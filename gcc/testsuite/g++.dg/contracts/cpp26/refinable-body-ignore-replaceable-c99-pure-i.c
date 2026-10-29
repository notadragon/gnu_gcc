/* Extra translation unit of refinable-body-ignore-replaceable-c99-pure.C,
   compiled as C: the C99 inline definition of f, its assertion ignored, and
   a caller compiled with it in view.  */

#include <contracts.h>

int g;

inline int f (int x)
{
  _ContractAssert (x > 0);	/* ignore */
  return x;
}

int use_g (int y)
{
  g = 1;
  int r = f (y);
  return r + g;
}
