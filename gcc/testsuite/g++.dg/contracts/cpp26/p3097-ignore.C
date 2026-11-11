// P3097: ignore semantic -- no wrapper, no contract checks, function still
// callable.  (Port of Clang clang/test/Contracts/Runnable/p3097-ignore.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int call_count = 0;

struct Base {
  virtual int f(int x) pre(x > 0) post(r: r > 0) { return x; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f(int x) override pre(x > -100) post(r: r > -100) { ++call_count; return x; }
};

int main() {
  Derived d;
  Base& b = d;

  // Under ignore: no contract checks (x=-50 violates both), function still runs.
  call_count = 0;
  int r = b.f(-50);
  if (r != -50) __builtin_abort();
  if (call_count != 1) __builtin_abort();
}
