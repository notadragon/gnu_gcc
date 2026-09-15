---
id: 6200-p3100-check-invalid-value
subject: 'c++: contracts: implicit invalid-value-load check (RUC_BOOL, RUC_ENUM)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check ids 12 and 13 -- `-fsanitize=bool` and `-fsanitize=enum` --
and the implicit contract assertion
`ub:conv.lval.valid.representation.bool.enum` that covers both.

One commit for the two wire ids, for the same reason as the shift check:
the language rule is one rule.  Reading an object of type `bool` or of an
unscoped enumeration type whose stored bits are not a valid value of that
type is one lvalue-to-rvalue conversion with an invalid representation; the
front end registers one group id for it and
`instrument_bool_enum_load_contract` handles both types in one function,
differing only in how it computes the valid range.  There is no
configuration that selects one and not the other.

The lowering reuses the sanitizer's raw-bits load and range test but changes
what happens on failure: instead of "report, then continue with the raw
value" it *substitutes a defined valid value* -- `false`, or an in-range
enumerator -- unconditionally, and for a checking semantic adds a
very-unlikely branch that runs the reaction for its side effect only.
`ignore` is the substitution with no branch.  As with signed overflow, this
is a codegen change and not a suppressed report.

The `ends_bb` path is the subtle part and has its own test
(`p3100-bool-enum-ends-bb.C`): a load can throw under
`-fnon-call-exceptions`, and an EH region is created by something as
ordinary as a local with a destructor, so the load is retargeted to produce
the raw bits and the substitution is built on the fall-through edge.
Bailing out there instead would behave as `assume` whatever was configured,
including for `ignore`, which must still substitute.

## Compile gap

None: the reaction resolver `implicit_invalid_value_reaction` is here, and
the langhooks, the reaction enumeration and the `pass_ubsan` driver that
calls `instrument_bool_enum_load_contract` are in `6000-p3100-core`.

The trap is the one the substitution creates.  Because `ignore` and
`observe` both replace an out-of-range value with a valid one, a program
that reads such a value observes a *different value* under P3100 than under
plain `-fsanitize=bool`, which leaves the raw bits alone.  That is
intentional -- it is what makes the operation defined -- but it is invisible
in the identifiers, and it means the check cannot be turned on for a
translation unit that relies on reading out-of-range enumerators.

## Contents

- gcc/contracts-routed-checks.def : /RUC_BOOL/, /RUC_ENUM/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-bool-enum-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-invalid-value* : *
- gcc/testsuite/g++.dg/ubsan/p3100-bool-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-enum-* : *
- gcc/ubsan.cc : @implicit_invalid_value_reaction, @instrument_bool_enum_load_contract, /lower an implicit invalid-value-load contract assertion/
