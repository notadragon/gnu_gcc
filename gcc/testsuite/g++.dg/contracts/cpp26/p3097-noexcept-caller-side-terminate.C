// P3097: the interface contracts of a noexcept virtual function, checked
// outside its body -- in the virtual-call wrapper, which is also how GCC
// checks a C-variadic virtual function (p3097-variadic-wrapper.C) and a
// virtual destructor -- terminate the program when the violation handler
// throws, as they would in the function's body; the exception never
// reaches the caller's catch.  Each call is made through a volatile
// function pointer, so the caller cannot know the callee does not throw
// and keeps its handler.  CASE selects the call: 0 (this file) the
// ordinary virtual function, 1 the C-variadic one
// (p3097-noexcept-caller-side-terminate-variadic.C), 2 the virtual
// destructor (p3097-noexcept-caller-side-terminate-dtor.C); the -O2 files
// run each at -O2.
//
// Mirror: in the llvm_llvm-project fork,
// clang/test/Contracts/Runnable/p3097-noexcept-caller-side-terminate.cpp
// (CLANG-834).
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <exception>

#ifndef CASE
#define CASE 0
#endif

static int handled;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  throw 42;
}

static void
on_terminate ()
{
  if (handled == 1)
    std::_Exit (0);
  std::abort ();
}

struct B
{
  virtual int f (int x) noexcept pre (x > 0) { return x; }
  virtual int v (int x, ...) noexcept pre (x > 0) { return x; }
  virtual ~B () noexcept pre (false) {}
};

static int call_f (B &b) { return b.f (-1); }		// the wrapper
static int call_v (B &b) { return b.v (-1, 0); }	// the variadic wrapper
static int call_d (B *b) { delete b; return 0; }	// the destructor's

int (*volatile fp) (B &);
int (*volatile dp) (B *);

int
main ()
{
  std::set_terminate (on_terminate);
  try
    {
#if CASE == 0
      fp = call_f;
      B b;
      fp (b);
#elif CASE == 1
      fp = call_v;
      B b;
      fp (b);
#else
      dp = call_d;
      dp (new B);
#endif
    }
  catch (...)
    {
      std::abort ();	// no exception may leave a noexcept function's contracts
    }
  std::abort ();
}
