// P3100: invalid bool/enum value load (ub:conv.lval.valid.representation).
// ignore -> the load yields a defined valid value (false / a valid enum value,
// here 0), no handler; observe -> handler runs (assertion_kind::implicit) then
// continues with that defined value.  Throwing observe is excluded from this
// middle-end check's allowed set; with -fcontracts-p4298 the best fit is
// noexcept_observe (nothrow handler that continues).
// (Clang mirror: clang/test/Contracts/p3100-invalid-value.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O -Wno-return-type" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-invalid-value.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

namespace cs = std::contracts;

enum A { B = -3, C = 2 };   // non-fixed; valid range [-4, 3]

static int calls = 0;
static bool all_implicit = true;
void handle_contract_violation (const cs::contract_violation& v) {
  ++calls;
  if (v.kind () != cs::assertion_kind::implicit)
    all_implicit = false;
}

namespace ign_ns {                    // ignore
  bool loadb (const bool* p) { return *p; }
  A    loade (const A* p)    { return *p; }
}
namespace obs_ns {                    // observe
  bool loadb (const bool* p) { return *p; }
}

int main () {
  if (sizeof (int) != sizeof (A) || sizeof (bool) != 1)
    return 0;
  bool badb; unsigned char cb = 4; __builtin_memcpy (&badb, &cb, 1);
  A bade; int ce = 9; __builtin_memcpy (&bade, &ce, sizeof (int));

  // ignore: defined valid value 0, no handler.
  if (ign_ns::loadb (&badb) != false) __builtin_abort ();
  if (ign_ns::loade (&bade) != (A) 0) __builtin_abort ();
  if (calls != 0) __builtin_abort ();

  // observe: handler runs, then continues with the defined value.
  if (obs_ns::loadb (&badb) != false) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
  if (!all_implicit) __builtin_abort ();

  // valid values pass through unchanged, no handler either way.
  bool okb = true; A oke = C;
  if (ign_ns::loadb (&okb) != true) __builtin_abort ();
  if (ign_ns::loade (&oke) != C) __builtin_abort ();
  if (obs_ns::loadb (&okb) != true) __builtin_abort ();
  if (calls != 1) __builtin_abort ();
}
