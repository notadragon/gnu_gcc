// gcc-14-contract-capture-note-garbage-location.cpp                  -*-C++-*-
//
// GCC-14 -- this is the real content of upstream **PR126041**, whose own
// framing is wrong in two ways (see below).
//
// When a `contract_assert' inside TWO NESTED LAMBDAS names a local of the
// enclosing function, GCC correctly rejects the implicit capture -- and then
// prints the accompanying "declared here" note at a GARBAGE line number:
//
//   min.cpp:5:44: error: 'x' is not implicitly captured by a contract assertion
//   min.cpp:-133821006:26: note: 'x' declared here
//                ^^^^^^^^^ different on every run
//
// THE ERROR IS CORRECT; ONLY THE NOTE'S LOCATION IS WRONG.  Clang rejects the
// same program -- "implicit capture of local entity 'x' is not allowed when
// used exclusively in contract assertions" -- so the diagnosis is right and
// agreed.  A capture that exists only because of a contract assertion would
// change the closure type, which [expr.prim.lambda.closure] does not allow.
// Confirmed by construction: odr-use `x' in the lambda body as well as in the
// assertion and the program is accepted.
//
// TWO CORRECTIONS TO THE REPORT'S FRAMING, both measured 2026-09-02:
//
//   * "structured bindings" -- NOT required.  The report unpacks a tuple with
//     `auto [x, y] = data;'.  Plain `int x = 1;' corrupts the note the same
//     way.
//   * "nested GENERIC lambda" -- NOT required.  Two nested NON-generic
//     lambdas are enough; neither needs to be a template.
//
// What IS required is two levels of lambda nesting, with the named entity
// belonging to the enclosing *function*.  A single lambda (generic or not) at
// one level prints the note correctly, and so does a name belonging to the
// immediately enclosing lambda -- in the report's own output `factor', a
// parameter of the outer lambda, is printed correctly at 8:34 while the
// structured bindings two levels out are garbage.
//
// THE VALUE IS NOT A FIXED UNDERFLOW.  It differs on every run
// (-58774350, -798448334, -976600654, -1033314894 on four consecutive
// invocations), so this is a read of uninitialised or already-released
// memory, not an off-by-N in a line-table computation.  The report's
// "line table underflow" reading is therefore probably the wrong model.
//
// PROVENANCE: UPSTREAM'S, measured 2026-09-02.
//
//   our branch   : garbage line number
//   g++ 16.2.0   : garbage line number
//   g++-trunk    : *** ICE ***  -- tree check: expected tree that contains
//                  'decl minimal' structure, have 'indirect_ref' in
//                  cp_parser_lambda_body, at cp/parser.cc:13851
//   clang        : correct diagnostic, correct location
//
// **UPSTREAM TRUNK HAS MADE THIS WORSE**: on the 2026-09-01 nightly the bad
// note has become an ICE.  Our branch is trunk of 2026-08-31 plus our delta
// and does not ICE.  Which of those two facts explains the difference is
// UNRESOLVED -- our delta does modify `cp_parser_lambda_body' (it calls
// cp_parser_late_contracts there), so it may be masking the ICE rather than
// predating it.  Settling that needs an upstream-only build at both dates.
// Either way the ICE is worth reporting: the reporter does not know about it.

int
main ()
{
  int x = 1;

  auto outer = [&] ()
    {
      auto inner = [&] () { contract_assert (x > 0); };
      inner ();
    };

  outer ();
}
