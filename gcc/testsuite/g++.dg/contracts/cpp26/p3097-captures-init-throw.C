// P3097+P3098: An exception thrown while initializing an INTERFACE
// (caller-facing) postcondition capture on a virtual call.  For a non-virtual
// function (see p3098-except-init.C) this is a post_capture / evaluation_exception
// contract violation: under observe the handler runs, the predicate is skipped,
// and execution continues (the function returns normally).  The interface
// postcondition capture of a virtual function behaves the same way.
//
// Previously BUG-2: GCC swallowed the exception silently on the virtual
// interface path -- the wrapper never emitted the capture initializer (its
// shared capture VAR_DECL had DECL_INITIAL destructively cleared by the base
// function's emission before the wrapper was copied).  Now the initializer is
// preserved and restored for the wrapper.  See testing-gap-catalogue.md sec 10.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cstdio>

struct Bad {
  Bad(int) { throw 42; }
  Bad(const Bad&) { throw 42; }
  ~Bad() { }
};

struct Base {
  virtual int f(int i)
    post [b = Bad(1)] (true)
  { return i; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f(int i) override { return i; }
};

int main() {
  Derived d;
  Base& b = d;
  // Under observe the capture-init exception becomes a post_capture violation
  // and execution continues, so f returns 10; the violation is reported.
  int r = b.f(10);
  if (r != 10) __builtin_abort();
  std::printf("done\n");
}

// { dg-output "contract violation in function .* at .*(\n|\r\n|\r)" }
// { dg-output ".assertion_kind: post_capture, semantic: observe, mode: evaluation_exception.*terminating: no.*(\n|\r\n|\r)" }
