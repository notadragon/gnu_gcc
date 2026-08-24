// N5008:
// basic.contract.eval/p17
// If a contract-violation handler invoked from the evaluation of a function contract assertion (9.4.1) exits via
// an exception, the behavior is as if the function body exits via that same exception.
// [Note 13 : If a contract-violation handler invoked from an assertion-statement (8.8)) exits via an exception, the search
// for a handler continues from the execution of that statement. — end note]
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe " }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <exception>
#include <cstdlib>

struct MyException{};

// The two things the note above actually promises, both of which this test
// used to run past without checking: the throwing handler's exception must
// propagate FROM THE ASSERTION STATEMENT -- so the statement after it does
// not execute -- and it must be catchable at the enclosing try.
static int handler_calls = 0;
static int reached_after = 0;
static int caught = 0;

void handle_contract_violation(const std::contracts::contract_violation& violation)
{
  ++handler_calls;
  throw MyException{};
}

void free_f(const int x) {
  try {
	  contract_assert(x>1);
	  ++reached_after;
  }
  catch(const MyException&){ ++caught; }
}

struct X
{
    void f(const int x) {
      try {
	  contract_assert(x>1);
	  ++reached_after;
      }
      catch(const MyException&){ ++caught; }
    }

    virtual void virt_f(const int x) {
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
  // The exception left the assertion statement, so nothing after it ran.
  if (reached_after != 0)
    __builtin_abort();
  if (caught != 3)
    __builtin_abort();
}
