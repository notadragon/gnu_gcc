// P3097: Postcondition evaluation order (implementation post before interface post).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int log_idx = 0;
static const char* log_comments[10];

void handle_contract_violation(const std::contracts::contract_violation& v) {
  if (log_idx < 10)
    log_comments[log_idx++] = v.comment();
}

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 100)
  { return x; }
};

struct Derived : Base {
  int f(int x) override
    pre(x > -100)
    post(r: r > 50)
  { return x; }
};

int main() {
  Derived d;
  Base& b = d;

  // x=5: both postconditions fail.
  // Order: impl post (r>50) fires first, then interface post (r>100).
  log_idx = 0;
  b.f(5);
  // Pre passes for both (5 > 0, 5 > -100).
  // Derived post fails: 5 > 50 is false.
  // Base post fails: 5 > 100 is false.
  if (log_idx != 2) __builtin_abort();
  // Verify ordering: first violation is impl post, second is interface post.
  if (std::strcmp(log_comments[0], "r > 50") != 0) __builtin_abort();
  if (std::strcmp(log_comments[1], "r > 100") != 0) __builtin_abort();
}
