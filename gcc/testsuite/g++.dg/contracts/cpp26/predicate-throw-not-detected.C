// A predicate that can throw only through a constructor, a dynamic_cast, a
// default argument of a noexcept callee, or a new-expression is evaluated
// inside the evaluation_exception catch, so the exception becomes a
// violation.
//
// Mirror: clang/test/Contracts/predicate-throw-constructor.cpp,
// predicate-throw-dynamic-cast.cpp, predicate-throw-default-arg.cpp and
// predicate-throw-new-array.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <new>
#include <typeinfo>

static int handled = 0;
void handle_contract_violation (const std::contracts::contract_violation& v)
{
  if (v.detection_mode ()
      == std::contracts::detection_mode::evaluation_exception)
    ++handled;
}

struct T { T (int) { throw 1; } bool ok () const noexcept { return true; } };
struct B { virtual ~B () = default; };
struct D : B {};
const B gb{};
int thrower () { throw 2; }
bool check (int, int = thrower ()) noexcept { return true; }

bool f1 (int x) pre (T (x).ok ()) { return true; }
bool f2 (int x) pre (&dynamic_cast<const D&> (gb) != nullptr) { return x; }
bool f3 (int x) pre (check (x)) { return true; }
bool f4 (int x) pre (new int[x] != nullptr) { return true; }

template <typename F>
static void expect_reported (F f)
{
  int before = handled;
  try { f (-1); } catch (...) { __builtin_abort (); }
  if (handled != before + 1)
    __builtin_abort ();
}

int main ()
{
  expect_reported (f1);
  expect_reported (f2);
  expect_reported (f3);
  expect_reported (f4);
}
