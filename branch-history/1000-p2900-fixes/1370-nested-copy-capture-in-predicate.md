---
id: 1370-nested-copy-capture-in-predicate
subject: "c++: contracts: P2900: a nested lambda's copy in a predicate has the entity's type"
depends: []
regenerates: []
fixes: []
---

## Rationale

In a contract predicate, a mutable lambda nested in a lambda in the
predicate may capture by copy what the enclosing lambda captured by
reference: `[&] { return [=] () mutable { return ++q; } (); } ()'.
[expr.prim.id.unqual] applies the capture rule before constification, so
`q' in the inner body names the inner closure's member, whose type is the
captured entity's -- `int' -- and which the mutable lambda may modify.  The
enclosing lambda's by-reference capture is `const int &' in the predicate
(`1070-lambda-capture-constify'), and `lambda_capture_field_type' took the
inner member's type from it.  It now looks through such a capture to the
entity when the const is one the entity does not have.  A namespace-scope
variable is not captured, and stays const.

(GCC-636 in `open-issues/'; decided valid in the notadragon_wg21 repository's
`DECISIONS.md', E32.)

## Compile gap

None.

## Contents

- gcc/cp/lambda.cc : /Capturing by copy what an enclosing lambda captured by reference:/
- gcc/testsuite/g++.dg/contracts/cpp26/nested-copy-capture-in-predicate.C : *
