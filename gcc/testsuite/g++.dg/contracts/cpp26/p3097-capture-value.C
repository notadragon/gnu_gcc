// P3097+P3098: the interface postcondition capture of a virtual function must
// capture the *correct* value across dispatch.  This pins the
// virtual-wrapper capture-init emission: a wrapper whose
// capture is left uninitialized reads 0, which a test expecting 0 cannot
// detect.  Here the captured value is non-zero, so an
// uninitialized capture is detected.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int shared = 100;
static int viol = 0;
void handle_contract_violation (const std::contracts::contract_violation&) { ++viol; }

struct Base {
  virtual int f () post [old = shared] (r: r == old + 1) { return shared; }
  virtual ~Base () = default;
};

struct Derived : Base {
  int f () override { shared += 1; return shared; }
};

int main () {
  Derived d;
  Base& b = d;
  // Interface captures old = 100; Derived returns 101; post 101 == 100 + 1 holds.
  // If the capture were uninitialized (0), the post would be 101 == 0 + 1 -> false.
  viol = 0;
  int r = b.f ();
  if (r != 101) __builtin_abort ();
  if (viol != 0) __builtin_abort ();

  // Now old = 101; Derived returns 102; post 102 == 101 + 1 holds.
  int r2 = b.f ();
  if (r2 != 102) __builtin_abort ();
  if (viol != 0) __builtin_abort ();
}
