# GCC-2: Forming the address of a member before construction begins is accepted

**Status:** Open
**Component:** c++ / constexpr
**Upstream Link:** --
**Affects:** measured against this branch's GCC and Clang builds
(2026-08-25), not an independent stock build; stock g++ 13.3 rejects the
program, but for an unrelated reason (virtual base classes were not yet
allowed in a constexpr constructor at all)

## Bug Report

The constant evaluator does not diagnose forming the address of a
non-static member or base before a non-trivial constructor begins
([class.cdtor]/1, /3). GCC additionally fails to diagnose even the one
shape Clang catches, because GCC never performs a dynamic virtual-base
conversion when the most-derived type is statically known -- it folds
straight to a constant offset. The following report text was drafted after
reading [class.cdtor] directly. (The table it refers to as "the table
above" is reproduced further down in this file.)

> **The report to write.**  Lead with the rule and the table, in that order:
>
> 1. [class.cdtor]/1 makes *referring to* any non-static member or base of
>    an object with a non-trivial constructor, before that constructor
>    begins, undefined -- with no carve-out for virtual versus non-virtual
>    bases and none distinguishing forming an address from reading.
>    [class.cdtor]/3 reinforces it for pointer formation specifically. Such
>    an expression is therefore not a core constant expression and must be
>    rejected.
> 2. The four-row table above, unedited. Rows (b) and (c) are the report:
>    two shapes, undefined for the same reason, accepted silently in a
>    constant expression. Row (a) is included because it shows the check is
>    *implementable* -- Clang already rejects it -- and row (d) because it
>    shows the evaluator does diagnose the analogous READ, so the gap is
>    specifically about address formation and not about lifetime tracking
>    in general.
> 3. State plainly that (a) is not a GCC-specific miss with a GCC-specific
>    cause. GCC folds the conversion to a constant offset and never consults
>    the object, so there is no dynamic-type operation to hang a check on; a
>    patch that only made (a) work would be a patch to the wrong place.
>
> Ask for the general check -- "referring to a member or base before
> construction begins", in the constant evaluator, covering address
> formation as well as reads -- and note the compatibility question that
> comes with it, since code that does this today compiles.

The "four-row table" referenced above (from the same source document):

| | Shape | GCC | Clang |
|---|---|---|---|
| a | virtual base member, form address | **accepts** | rejects |
| b | non-virtual base member, non-trivial ctor, form address | **accepts** | **accepts** |
| c | direct member, non-trivial ctor, form address | **accepts** | **accepts** |
| d | virtual base member, read the value | rejects | rejects |

## Reproducer

See [`gcc-02-constexpr-vbase-before-ctor.cpp`](gcc-02-constexpr-vbase-before-ctor.cpp)
in this directory.

## Our Fix

None -- deliberately. This is a core-language constant-evaluator change
owed to the compiler, not something to patch on this branch. This is not
contracts-related at all (pure core-language constexpr; found only because
contracts work led here), so it is filable against both GCC and LLVM today.
Clang's counterpart is CLANG-1 in the `llvm_llvm-project` fork, whose row
(a) has already been fixed there -- that fix does not close rows (b) or (c)
on Clang either.

## Notes

Scope decision taken by the user, 2026-09-04: report the full gap (rows (b)
and (c)), not just the one row ((a)) where GCC and Clang currently differ --
narrowly reporting only (a) would describe the symptom that happens to be
visible rather than the actual defect, and would misdescribe GCC's reason
for missing (a). Send the same report to LLVM as well, adjusted: Clang
rejects (a) and accepts (b) and (c), so its gap is the same two rows, and
its (a) is incidental to the virtual-base-offset path rather than a general
check. Tracked as CLANG-9 in the `llvm_llvm-project` fork. See
`notadragon_wg21`'s
`src/pubs/impl/p3850impl/gcc-bugs/pending_reports.md` (GCC-2 entry) for the
full analysis this report is drawn from, including "Why GCC does not catch
(a)".
