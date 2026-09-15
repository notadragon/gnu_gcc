// Throwing violation handler in an assert check in a noexcept function
// can be caught by the function.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <exception>
#include <cstdlib>

struct MyException{};

// The functions are noexcept, as the title says: the handler's exception is
// caught inside the function and so never escapes it.  They were not marked
// noexcept before, and nothing observed the outcome either, so the test ran
// to completion no matter what happened.
static int handler_calls = 0;
static int reached_after = 0;
static int caught = 0;

void handle_contract_violation(const std::contracts::contract_violation& violation)
{
  ++handler_calls;
  throw MyException{};
}

void free_f(const int x) noexcept {
  try {
	  contract_assert(x>1);
	  ++reached_after;
  }
  catch(const MyException&){ ++caught; }
}

struct X
{
    void f(const int x) noexcept {
      try {
	  contract_assert(x>1);
	  ++reached_after;
      }
      catch(const MyException&){ ++caught; }
    }

    virtual void virt_f(const int x) noexcept {
      try {
	  contract_assert(x>1);
	  ++reached_after;
      }
      catch(const MyException&){ ++caught; }
    }

};

int main()
{
  free_f(-42);

  X x;
  x.f(-42);
  x.virt_f(-42);

  if (handler_calls != 3)
    __builtin_abort();
  if (reached_after != 0)
    __builtin_abort();
  if (caught != 3)
    __builtin_abort();


}
