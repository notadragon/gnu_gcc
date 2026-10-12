# GCC-644: a local named in typeid of a polymorphic glvalue in a lambda is not captured

**Status:** Open (upstream); open here
**Component:** c++ (lambdas, typeid)
**Keywords (ours -- upstream sets its own):** `accepts-invalid`, regression
**Upstream Link:** None found (searched 2026-10-06, twice: see "Upstream
search" below).  Related: PR125886 and PR127132 (both fixed; the commits that
introduced and adjusted the two-pass parse), PR116385 (the non-polymorphic
case, fixed by the same work).
**Affects:** measured 2026-10-06: stock trunk (`17.0.0 20261005`, the CE
nightly at `8802cd9fdbe`) and this branch accept; 16.2.0 and 13.4.0 reject
("'x' is not captured").  A regression in GCC 17.

**Bugzilla Summary:** [17 Regression] local variable in typeid of a
polymorphic glvalue in a lambda without capture-default is not diagnosed

## Reproducer

[`gcc-644-typeid-polymorphic-capture.cpp`](gcc-644-typeid-polymorphic-capture.cpp),
at `-std=c++20 -fsyntax-only`:

    #include <typeinfo>
    struct B { virtual ~B (); };
    void f (B x) { auto l = [] { return &typeid (x); }; }

Compiles; it links and runs too, with `x` never captured.  [expr.typeid]
makes the operand an unevaluated operand only when it is not a glvalue of
polymorphic class type; here it is, so `x` is odr-used.  And
[expr.prim.lambda.capture]/7 decides capture as though enclosing typeid
expressions were ignored, so `x` must be captured whatever typeid does.
Clang rejects ("variable 'x' cannot be implicitly captured in a lambda with
no capture-default specified").

## Why

r17-2200 (`621661f7d2e`, PR125886) parses a typeid operand twice and keeps
the evaluated parse only for "a glvalue of polymorphic class type whose
dynamic type is not known at compile time"; a variable of class type has a
known dynamic type, so the operand is treated as unevaluated and the outer
variable reference never triggers capture.  `a310db3ba39` (PR127132) fixed
the generic-lambda half of the capture question but not this one.  The
standard has no "dynamic type known" exemption for evaluation, nor for
capture.

## Where (M5-GCC investigation, 2026-10-06)

* `cp_parser_postfix_expression` (`parser.cc`, the `RID_TYPEID` case)
  parses the operand under `cp_unevaluated`, then re-parses it evaluated
  only if `typeid_evaluated_p` says so.
* `typeid_evaluated_p` (`rtti.cc`) requires `!resolves_to_fixed_type_p
  (exp)`.  For a variable (or by-value parameter) of class type the dynamic
  type is fixed, so the answer is "unevaluated" although the operand is a
  glvalue of polymorphic class type ([expr.typeid]/4 makes only a
  non-polymorphic or non-glvalue operand unevaluated).  The same predicate
  feeds `build_typeid` and the `TYPEID_EXPR` case of `tsubst_expr`, where
  "fixed type" is a legitimate code-generation shortcut, not a rule about
  evaluation.
* During the first (probe) parse, `process_outer_var_ref` (`semantics.cc`,
  `in_typeid_probe`) attempts the capture, which fails without a
  capture-default, and defers the diagnostic to the evaluated re-parse --
  which never happens, so nothing is diagnosed.  With `[=]` the capture
  made in the probe survives, which is why only the no-capture-default
  case is wrong.  Independently of capture, [expr.prim.lambda.capture]/7
  decides implicit capture as if enclosing typeid expressions were
  ignored, so even a truly unevaluated typeid operand naming `x` makes `x`
  captured by a capture-default lambda.

**Regressing commit:** r17-2200 (`621661f7d2e`, PR125886), identified by
source reading, not bisected: it introduced the two-pass parse and
`typeid_evaluated_p`; 16.2.0 (single evaluated parse) rejects; r17-3993
(`a310db3ba39`, PR127132) adjusted only the generic-lambda case.

## Upstream search (2026-10-06)

Bugzilla REST: quicksearch `ALL typeid lambda`, `ALL typeid capture`, `ALL
typeid odr-use`, `ALL typeid polymorphic unevaluated`, `ALL typeid not
captured`; component c++ since 2026-07-01 with summary containing
`17 Regression` and `lambda` / `captur` / `typeid` / `odr`, summary
containing `captur` / `typeid`; any product since 2026-07-01 with a comment
containing `typeid`.  No report of this bug.  Found:
* PR125886 (fixed by r17-2200) and PR127132 (fixed by r17-3993, comment 5:
  the capture decision and `typeid_evaluated_p` share
  `cp_unevaluated_operand`) -- the two follow-ups of this work, both
  closed;
* PR116385 (UNCONFIRMED) and PR68604 (NEW), both listed as fixed by
  r17-2200's commit message but still open;
* PR123594 (nested generic lambdas and typeid ICE, a 14-17 regression) and
  PR127576 (typeid with modules) -- unrelated.

## Found by

The EDG fork's EDG-58 (typeid of a by-value polymorphic parameter in a
postcondition odr-uses it), cross-checked 2026-10-02.  Our postcondition
const check is right on that shape (`postcondition-typeid-polymorphic-param.C`);
only the lambda capture is wrong.
