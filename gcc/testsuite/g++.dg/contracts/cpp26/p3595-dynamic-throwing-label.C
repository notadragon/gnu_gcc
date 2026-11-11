// P3595, DECISIONS.md E137: the control for p3595-dynamic-nothrow-label-*.C.
// Under a label that allows enforce, a dynamic selection is unchanged: an
// invalid selection is reported, without evaluating the (true) predicate,
// as an enforce violation, and a selected observe is evaluated with enforce
// (the label allows no observe); in both the handler's exception leaves
// the check and is caught.  p3595-dynamic-throwing-label-no-p4298.C: the
// same without -fcontracts-p4298 (E138's quick_enforce trap is only for a
// label with no throwing semantic).
// Mirror: in the llvm_llvm-project fork,
// clang/test/Contracts/Runnable/p3595-dynamic-throwing-label.cpp.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-dynamic-nothrow-label.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct throwing_t {
  using assertion_control_object = throwing_t;
  static constexpr evaluation_semantic_set allowed_semantics =
    {evaluation_semantic::enforce, evaluation_semantic::quick_enforce};
};
constexpr throwing_t throwing_label{};

static int choice;

evaluation_semantic
p3595_nothrow_sel ()
{
  return evaluation_semantic (choice);
}

static int handled;
static evaluation_semantic seen;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  ++handled;
  seen = v.semantic ();
  throw 42;
}

void f (int x) pre<throwing_label> (x > 0) { }

static bool
caught (int x)
{
  try
    {
      f (x);
    }
  catch (int)
    {
      return true;
    }
  return false;
}

int main ()
{
  choice = 42;		// not a semantic: an invalid selection
  if (!caught (1) || handled != 1 || seen != evaluation_semantic::enforce)
    std::abort ();
  choice = (int) evaluation_semantic::observe;
  if (!caught (-1) || handled != 2 || seen != evaluation_semantic::enforce)
    std::abort ();
}
