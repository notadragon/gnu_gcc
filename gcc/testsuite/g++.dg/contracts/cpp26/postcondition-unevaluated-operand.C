// A parameter named inside an UNEVALUATED operand of a postcondition
// predicate is not odr-used, so [dcl.contract.func] does not require it to be
// const.
//
// [dcl.contract.func]: "If the predicate of a postcondition assertion of a
// function f odr-uses a non-reference parameter of f, that parameter and the
// corresponding parameter on all declarations of f shall have const type."
// [basic.def.odr]/5 odr-uses a variable named by a POTENTIALLY-EVALUATED
// expression; the operand of decltype, of sizeof, of noexcept, and the
// requirements of a requires-expression are unevaluated, so naming a
// parameter there is not an odr-use.
//
// check_postcondition_odr_use_r walks the finished predicate (PR126897) and
// stops at the unevaluated operands it knows about.  cp_walk_subtrees enters
// exactly three kinds of subtree under `cp_unevaluated`: the operand of
// SIZEOF_EXPR/ALIGNOF_EXPR/NOEXCEPT_EXPR, DECLTYPE_TYPE_EXPR, and
// REQUIRES_EXPR_REQS.  Only the first group was guarded, so a parameter
// reached through a decltype or a requires-expression was wrongly marked
// odr-used -- rejecting programs stock g++ accepts, and, because the same
// flag drives the coroutine rule, telling a coroutine its parameter was
// odr-used when it was not.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wno-unused-value" }

#include <coroutine>

template <typename T> bool g ();

// ---------------------------------------------------------------- decltype

void dt (int p) post (g<decltype (p)> ()) {}

// Parenthesized: still an unevaluated operand, and decltype((p)) is a
// different type from decltype(p), which is exactly why the operand's
// spelling must not decide whether the const rule fires.
void dt_paren (int p) post (g<decltype ((p))> ()) {}

// The dependent forms are the ones that regressed: with a concrete type the
// decltype has folded away before the walk runs, so nothing was left to find.
template <typename T> void dt_dep (T p) post (g<decltype (p)> ()) {}
template void dt_dep<int> (int);

template <typename T> void dt_dep_paren (T p) post (g<decltype ((p))> ()) {}
template void dt_dep_paren<int> (int);

// ------------------------------------------------------- requires-expression

// This one was rejected even with a concrete type: a REQUIRES_EXPR survives
// into the finished predicate whether or not anything in it is dependent.
void req (int p) post (requires { +p; }) {}

template <typename T> void req_dep (T p) post (requires { +p; }) {}
template void req_dep<int> (int);

// ------------------------------------- already-guarded operands, as controls

void sz (int p) post (sizeof (p) > 0) {}

template <typename T> void sz_dep (T p) post (sizeof (p) > 0) {}
template void sz_dep<int> (int);

void nx (int p) post (noexcept (+p) || true) {}

template <typename T> void nx_dep (T p) post (noexcept (+p) || true) {}
template void nx_dep<int> (int);

// ------------------------------------------------------------- redeclaration

// PR127196 comment 1.  The declarations differ in top-level const, which is
// ordinary C++ where the parameter is not odr-used, and only one of them
// carries the postcondition -- so there is no second predicate to mismatch
// against either.  Well-formed.
template <typename T> void redecl (T p) post (g<decltype (p)> ());
template <typename T> void redecl (T const p) {}
template void redecl<int> (int);

// ---------------------------------------------------------------- coroutine

struct Task
{
  struct promise_type
  {
    Task get_return_object () { return {}; }
    std::suspend_always initial_suspend () noexcept { return {}; }
    std::suspend_always final_suspend () noexcept { return {}; }
    void return_void () { }
    void unhandled_exception () { }
  };
};

// A coroutine may not odr-use a non-reference parameter in a postcondition,
// and this does not: naming it in an unevaluated operand is fine.
template <typename T> Task coro (T p) post (g<decltype (p)> ()) { co_return; }
template Task coro<int> (int);

// -------------------------------------------------- controls that must fail

// A real odr-use is still caught, dependent or not -- loosening the walk must
// not lose the rule it exists to enforce.
void odr_use (int p) post (p > 0) {} // { dg-error "value parameter used in a postcondition must be const" }

// The dependent form draws TWO diagnostics on the one line, at different
// columns: the check on the declaration, and the walk over the substituted
// predicate.  Stock g++ trunk does the same, measured before writing this
// expectation, so it is pre-existing and not something this test's fix
// introduced.
template <typename T>
void odr_use_dep (T p) post (p > 0) {} // { dg-error "value parameter 'p' used in a postcondition must be const" }
// { dg-error "a value parameter used in a postcondition must be const" "" { target *-*-* } .-1 }
template void odr_use_dep<int> (int);  // { dg-message "required from here" }

// An odr-use sitting alongside an unevaluated one is still an odr-use.
void mixed (int p) post (g<decltype (p)> () && p > 0) {} // { dg-error "value parameter used in a postcondition must be const" }

// Const parameters are accepted in every one of these positions.
void ok_const (const int p) post (g<decltype (p)> () && p > 0) {}
