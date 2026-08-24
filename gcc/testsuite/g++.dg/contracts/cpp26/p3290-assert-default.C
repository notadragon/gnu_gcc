// P3290: assert macro unchanged without __STDC_WANT_ASSERT_USES_CONTRACTS__.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290" }

#include <cassert>

int main() {
  assert(1 == 1);
}
