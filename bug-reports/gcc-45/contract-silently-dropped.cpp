// A function-contract-specifier that follows a parameter list sitting inside
// a trailing-return type-id binds to that parameter list rather than to the
// declarator, and is then discarded.  No call is ever checked, and there is
// no diagnostic at any point.
//
// A function-contract-specifier-seq follows the complete declarator
// ([dcl.decl.general]/1, and [dcl.contract.func]/3 for what it attaches to),
// so all three preconditions below belong to the function being declared and
// all three are owed a violation at the calls in main.
//
// The classic spelling is the control.  It is the same function, written
// without a trailing return type, and its contract is attached and reported
// -- which is what shows the other two should be.  (The classic spelling has
// its own defect, in the scope its predicate is parsed in, and that is a
// separate reproducer: contract-predicate-wrong-scope.cpp.  The predicates
// here name no parameter, so that defect cannot show and this file measures
// the drop and nothing else.)
//
// g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
//     -o a.out contract-silently-dropped.cpp
// ./a.out
//
// Expected: three violations, then "done".
// Actual:   one, from returns_fn_classic, then "done".
//
// This file must be run.  Every declaration in it compiles clean, so a
// -fsyntax-only measurement reads the same before and after a fix and can
// show neither the bug nor its repair.

#include <cstdio>

static int callee (int) { return 0; }

static bool ok = false;

// Control: the classic spelling.  Attached and reported.

int (*returns_fn_classic (int)) (int) pre (ok);

int (*returns_fn_classic (int)) (int) { return callee; }

// The same declaration with a trailing return type.  The parameter list is
// now inside the type-id, so the contract binds to an abstract declarator
// that never becomes a function, and is dropped.

auto returns_fn_trailing (int) -> int (*) (int) pre (ok);

auto returns_fn_trailing (int) -> int (*) (int) { return callee; }

// A postcondition in the same position, so a fix that only handles `pre` is
// still caught.

auto returns_ref_trailing (int) -> int (&) (int) post (r : ok);

auto returns_ref_trailing (int) -> int (&) (int) { return callee; }

int main ()
{
  returns_fn_classic (0);
  returns_fn_trailing (0);
  returns_ref_trailing (0);
  std::puts ("done");
}
