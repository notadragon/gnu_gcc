// A postcondition capture initializer that throws under noexcept_observe
// reports a post_capture violation through the _noexcept entry point, and
// the program then continues; post-capture-noexcept-outlined.C does the same
// with the checks outlined.
//
// Mirror: clang/test/Contracts/post-capture-noexcept-link.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using namespace std::contracts;

int g (int x) { if (x) throw 1; return x; }
int f (int x) post [c = g (x)] (c == 0) { return x; }

static int reports = 0;
void handle_contract_violation (const contract_violation &v)
{
  if (v.semantic () == evaluation_semantic::noexcept_observe
      && v.detection_mode () == detection_mode::evaluation_exception)
    ++reports;
}

int main ()
{
  if (f (0) != 0 || reports != 0)
    __builtin_abort ();
  if (f (1) != 1 || reports != 1)
    __builtin_abort ();
}
