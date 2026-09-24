// P3097+P3098: Postcondition captures on a virtual function reached through a
// virtual (diamond) base.  Verify the single shared base subobject's capturing
// interface postcondition is checked once with correct call-time captures.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int state = 0;

struct Base {
  virtual int f()
    post [old = state] (r: r == old + 1)
  { return ++state; }
  virtual ~Base() = default;
};

struct Left : virtual Base { };
struct Right : virtual Base { };

struct Join : Left, Right {
  int f() override { return state += 3; }  // disagrees with Base post (+1)
};

int main() {
  Join j;
  Base& b = j;  // unambiguous: single virtual Base subobject

  // Captures old = 0, Join::f sets state = 3.
  // Base interface post: 3 == 0 + 1 -> false -> exactly one violation
  // (single shared subobject, checked once).
  state = 0;
  violation_count = 0;
  if (b.f() != 3) __builtin_abort();
  if (violation_count != 1) __builtin_abort();
}
