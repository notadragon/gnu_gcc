// GCC-49: a postcondition with a result name, on a function whose return
// type deduction never completes, trips an assertion during genericization.
//
// g++ -std=c++26 -fcontracts -fsyntax-only postcondition-undeduced-result-ice.cpp
//
// Expected: the one error below, and nothing else.
// Actual:   internal compiler error: in check_noexcept_r, at cp/except.cc:1063
//
// On a compiler built without checking the same abort is reported as
// "confused by earlier errors, bailing out" against the line of the return
// statement, because diagnostics/context.cc converts an ICE into a fatal
// error once any error has been emitted.  The two spellings are the same
// failure; only one of them says so.  That line is printed on the ICE path
// and nowhere else, so seeing it is proof enough -- do not reach for -dH to
// "unmask" it, which aborts on any error at all and so proves nothing.
//
// THE BUG: four things are required together, and each is measured below in a
// control that removes exactly one of them:
//
//   * a postcondition,
//   * with a result name,
//   * on a function with a deduced return type whose deduction never
//     completes, and
//   * a call in the predicate.
//
// CONTROLS -- each of these is diagnosed cleanly, with no abort.  They cannot
// live in this file, because the abort ends the translation unit before
// anything after it is compiled:
//
//   bool check (bool);
//   class S { bool f () post (r: check (r)) { return e; } };  // concrete return
//   class S { auto f () post (true)         { return e; } };  // no result name
//   class S { auto f () post (r: r)         { return e; } };  // no call
//   bool g (); auto f () post (r: g ()) { return true; }      // deduction works
//
// The failing deduction need not be a lookup failure.  This shape aborts the
// same way, with no undeclared name anywhere in it:
//
//   bool g (); auto f () post (r: g ()) { return f (); }
//
// which is what shows the trigger is "deduction never completed", not
// "something in the body was undeclared".

bool check (bool b) { return b; }

class S
{
  auto f ()
    post (r: check (r))
  { return e; }   // error: 'e' was not declared in this scope
};
