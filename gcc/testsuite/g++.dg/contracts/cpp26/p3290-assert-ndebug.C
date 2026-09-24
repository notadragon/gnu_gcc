// P3290: assert disabled with NDEBUG even if __STDC_WANT_ASSERT_USES_CONTRACTS__ is set.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3290 -DNDEBUG -D__STDC_WANT_ASSERT_USES_CONTRACTS__" }

#include <cassert>

int main() {
  assert(1 == 2);  // Should be compiled out due to NDEBUG.
}
