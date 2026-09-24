// P3097+P3098: A qualified (non-polymorphic) call to a capturing virtual
// function bypasses dispatch, so only the named level's captures/postcondition
// run.  Contrast a qualified Base::f() call with a polymorphic call on the
// same object.
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

struct Derived : Base {
  int f() override
    post [old = state] (r: r == old + 2)
  { return state += 2; }
};

int main() {
  Derived d;
  Base& b = d;

  // Qualified call d.Base::f(): no virtual dispatch.  Only Base::f runs and
  // only Base's capturing post is checked: captures old = 0, state -> 1,
  // 1 == 0 + 1 -> true.  Derived's post is NOT involved.
  state = 0;
  violation_count = 0;
  if (d.Base::f() != 1) __builtin_abort();
  if (violation_count != 0) __builtin_abort();

  // Polymorphic call b.f(): both implementation (Derived) and interface (Base)
  // capturing posts run.  captures old = 1 (both), Derived sets state = 3.
  //   Derived post: 3 == 1 + 2 -> true.
  //   Base interface post: 3 == 1 + 1 -> false -> one violation.
  violation_count = 0;
  if (b.f() != 3) __builtin_abort();
  if (violation_count != 1) __builtin_abort();
}
