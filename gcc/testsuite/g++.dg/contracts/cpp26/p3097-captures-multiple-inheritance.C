// P3097+P3098: Postcondition captures on virtual functions under multiple
// inheritance.  A class derives from two bases, each declaring a distinct
// capturing virtual function; verify each interface's captures/postcondition
// bind to the correct base subobject across dispatch.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violation_count = 0;

void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violation_count;
}

static int state_a = 0;
static int state_b = 0;

struct BaseA {
  virtual int fa()
    post [old_a = state_a] (r: r == old_a + 1)
  { return ++state_a; }
  virtual ~BaseA() = default;
};

struct BaseB {
  virtual int fb()
    post [old_b = state_b] (r: r == old_b + 10)
  { return state_b += 10; }
  virtual ~BaseB() = default;
};

struct Derived : BaseA, BaseB {
  int fa() override { return state_a += 2; }   // disagrees with BaseA post (+1)
  int fb() override { return state_b += 10; }  // agrees with BaseB post (+10)
};

int main() {
  Derived d;
  BaseA& ba = d;
  BaseB& bb = d;

  // fa via BaseA&: captures old_a = 0, Derived::fa sets state_a = 2.
  // BaseA interface post: 2 == 0 + 1 -> false -> violation.
  state_a = 0;
  violation_count = 0;
  if (ba.fa() != 2) __builtin_abort();
  if (violation_count != 1) __builtin_abort();

  // fb via BaseB&: captures old_b = 0, Derived::fb sets state_b = 10.
  // BaseB interface post: 10 == 0 + 10 -> true -> no violation.
  state_b = 0;
  violation_count = 0;
  if (bb.fb() != 10) __builtin_abort();
  if (violation_count != 0) __builtin_abort();
}
