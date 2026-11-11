#include "refinable-2.h"

[[gnu::noinline]] int
mid (int x)
{
  return leaf (x) - 1;
}
