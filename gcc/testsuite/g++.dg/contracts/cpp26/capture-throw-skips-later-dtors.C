// Under observe, a capture initializer that throws is reported and the
// postcondition is skipped; the captures constructed before it are destroyed,
// and the one that threw and the ones after it, never constructed, are not.
//
// Mirror: clang/test/Contracts/capture-throw-skips-later-dtors.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int ctors, dtors;
struct Good {
  Good () { ++ctors; }
  Good (const Good &) { ++ctors; }
  ~Good () { ++dtors; }
};
struct Bad {
  Bad () { throw 1; }
  Bad (const Bad &) { throw 1; }
  ~Bad () { ++dtors; }
};

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
}

int f (int x) post [g = Good (), b = Bad (), h = Good ()] (false) { return x; }

int main ()
{
  f (1);
  if (reports != 1 || ctors != 1 || dtors != 1)
    __builtin_abort ();
}
