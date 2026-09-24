// A postcondition with a result name, on a function whose return type
// deduction never completes, must be diagnosed and no more.  It used to abort
// the compiler:
//
//   internal compiler error: in check_noexcept_r, at cp/except.cc:1063
//
// Such a predicate is parsed with processing_template_decl raised, because the
// result variable's type is `auto' while it is parsed, so a call in it is
// built in template form.  rebuild_postconditions makes it concrete once the
// return type is known, and declines while the type is undeduced -- and when
// the body never produces a type, nothing comes back to do it.  Genericization
// then asked expr_noexcept_p whether the predicate could throw, and
// check_noexcept_r asserted on the unresolved callee.
//
// A call in the predicate was part of the trigger, because check_noexcept_r
// asserts on nothing else; the rows below keep one so that a regression is
// reached rather than merely risked.
//
// Tracked as GCC-49 in this repository's bug-reports/ and upstream as
// PR127450, where it is still open.  The mirror is
// clang/test/Contracts/OpenBugs/postcondition-undeduced-result.cpp, which
// pins that Clang has always rejected these cleanly.
//
// Every row was a separate abort before the fix, and they are kept apart
// because they fail deduction for genuinely different reasons -- only the
// first is the shape PR127450 reports.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

bool check (bool b) { return b; }
bool g ();

// ---- Deduction fails because the returned expression is ill-formed. ------
namespace undeclared {
  class S {
    auto f ()
      post (r: check (r))
    { return e; } // { dg-error "'e' was not declared in this scope" }
  };
}

// ---- Deduction fails with no undeclared name anywhere, which is what shows
//      the trigger is "deduction never completed" rather than "the body
//      mentioned something undeclared". --------------------------------------
namespace before_deduction {
  auto f ()
    post (r: g ())
  { return f (); } // { dg-error "before deduction of 'auto'" }
}

// ---- No return statement at all.  The contract-specific diagnostic IS
//      emitted here, by rebuild_postconditions once the auto-to-void fallback
//      in finish_function has run -- and the compiler aborted anyway, after
//      saying the right thing.  That is what showed invalidating the contract
//      is not enough: the checks have already been spliced into the body by
//      then, and genericization walks them regardless. ----------------------
namespace no_return {
  auto f ()
    post (r: g ()) // { dg-error "function does not return a value to test" }
  { }
}

// ---- auto*, which cannot deduce to void, so the fallback diagnoses instead
//      of applying. ---------------------------------------------------------
namespace auto_pointer {
  auto *f ()
    post (r: g ())
  { } // { dg-error "no return statements in function returning 'auto\\*'" }
      // { dg-warning "no return statement in function returning non-void" "" { target *-*-* } .-1 }
}

// ---- decltype(auto), the other deduced-return spelling. -------------------
namespace decltype_auto {
  decltype (auto) f ()
    post (r: g ())
  { return e; } // { dg-error "'e' was not declared in this scope" }
}

// ---- Inside a template, reached through instantiation.  Two diagnostics
//      here, not one: the erroneous return leaves current_function_returns_value
//      unset, so finish_function's auto-to-void fallback applies and the
//      result name is then diagnosed against void as well. --------------------
namespace in_template {
  template <class T>
  auto f (T)
    post (r: g ()) // { dg-error "function does not return a value to test" }
  { return e; } // { dg-error "'e' was not declared in this scope" }

  void use () { f (1); }
}

// ---- On a lambda, whose operator() takes a different path to its contracts.
namespace in_lambda {
  auto l = [] ()
    post (r: g ())
  { return e; }; // { dg-error "'e' was not declared in this scope" }
}

// ---- Two postconditions: the second must not be reached in a broken state
//      either, and each is declined independently. --------------------------
namespace two_postconditions {
  auto f ()
    post (r: g ())
    post (r2: check (r2))
  { return e; } // { dg-error "'e' was not declared in this scope" }
}

// ---- Controls.  These are well formed and must stay accepted: declining to
//      emit a check is the right answer only for a function that is already
//      being rejected, and a silently dropped postcondition would be a far
//      worse bug than the abort this replaces. -------------------------------
namespace controls {
  // Deduction succeeds, so the predicate is substituted normally.
  auto deduced ()
    post (r: check (r))
  { return true; }

  // No result name, so the predicate was never a template tree -- and the
  // return type is still undeduced when the checks are spliced, which is why
  // the fix cannot simply key off "the return type is auto here".
  auto no_result_name ()
    post (g ())
  { }

  // Concrete return type: the late-parse path builds the result variable with
  // the real type and never raises processing_template_decl at all.
  bool concrete ()
    post (r: check (r))
  { return true; }
}
