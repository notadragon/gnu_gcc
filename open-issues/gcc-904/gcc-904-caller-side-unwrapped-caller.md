# GCC-904: a caller with no caller-side check is not marked

**Kind:** deferred (DECISIONS.md E136, answer (b), 2026-10-24)
**Status:** Open -- documented limitation of the refinable-body mark;
filed by M6d-GCC
**Affects:** `mark_contract_wrapper_caller` (`gcc/cp/cp-gimplify.cc`), the
refinable-body rule for caller-side evaluations (P3595 caller-side
configuration, upstream's `-fcontracts-client-check=`) and P3097 wrappers
**Clang:** llvm_llvm-project CLANG-832 (Clang has no caller-side checking;
its form, if any, is a P3097 wrapper decided per unit)

## Symptom

[`open-bug-caller-side-unwrapped.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-caller-side-unwrapped.C)
(with `-k.cc`, `-u.cc` and a configuration file): `use_g (-5)` calls
`std::terminate` instead of returning -1 from its `catch`.

* `-u.cc` defines `f`, whose precondition is ignored on its own side, so
  `f` cannot throw, and the inline `g`, which calls `f` with no caller-side
  check -- no wrapper, so `g` holds no contract evaluation, is not marked,
  and is found nothrow; `use_g`, compiled there, drops its landing pad.
* `-k.cc`, linked first, defines the `g` the linker keeps, whose call of
  `f` is checked caller-side with enforce; the violation handler throws.

With the call in `-u.cc` checked caller-side too, the program returns -1
(measured with the M6b install, 2026-10-24).

## Cause

Whether a call is checked on the caller's side is decided by each
translation unit.  The refinable-body mark (M4-GCC) covers a function
that calls a contract wrapper, since the wrapper's evaluations are part of
the caller's behaviour, but a function compiled where none of its calls is
checked has no wrapper and nothing to mark it by.  Another unit's
definition of the same inline function may check those calls, with a
handler that throws or writes memory, and the linker may keep it.

## Disposition

Deferred (E136 (b)): caller-side checking, and P3097, must be configured
alike in every unit that defines the same inline function, as the
refinable-body design's possible "same configuration" flag would assume.
The alternative, (a) -- marking every function that calls a function with
precondition or postcondition specifiers, or a virtual function with
contracts -- would take facts from a large part of all inline code.
Documented at `mark_contract_wrapper_caller` and in
`refinable-body-gcc-audit.md` Section J.
