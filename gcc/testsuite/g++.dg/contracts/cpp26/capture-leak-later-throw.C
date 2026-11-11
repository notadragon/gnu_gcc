// A postcondition capture is initialized in lexical order with the
// preconditions, and an exception from a lexically later precondition's
// violation handler (f), or from a later postcondition's capture initializer
// with a throwing post_capture handler (f2), destroys the captures already
// made.  capture-leak-later-throw-outlined.C does the same with the checks
// outlined.
//
// Mirror: clang/test/Contracts/capture-leak-later-throw.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int live = 0;
struct Good {
  Good () { ++live; }
  Good (const Good&) { ++live; }
  ~Good () { --live; }
};
struct Bad { Bad () { throw 1; } };

void handle_contract_violation (const std::contracts::contract_violation&)
{
  throw 7;
}

int f (int i)
  post [g = Good ()] (true)
  pre (i > 0)
{
  return i;
}

int f2 (int i)
  post [g = Good ()] (true)
  post [b = Bad ()] (true)
{
  return i;
}

int main ()
{
  try { f (-1); } catch (int) {}
  if (live != 0)
    __builtin_abort ();
  try { f2 (1); } catch (int) {}
  if (live != 0)
    __builtin_abort ();
}
