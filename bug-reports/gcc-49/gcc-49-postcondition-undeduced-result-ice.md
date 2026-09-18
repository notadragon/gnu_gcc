# GCC-49: A postcondition with a result name on a function whose return type deduction fails aborts the compiler

**Status:** Fixed here
**Resolved by:** `0170-postcondition-undeduced-result-ice`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`,
`error-recovery`
**Upstream Link:**
[PR127450](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127450) -- filed
2026-09-17 by a third party (Yuancheng Jiang, found by `fusion-fuzz`),
UNCONFIRMED.  It is the only report: Bugzilla quicksearch for
`check_noexcept_r`, `expr_noexcept_p` and `build_contract_check` (2026-09-17)
returns PR127450 and nothing else, plus our own PR127255 and PR125904 on the
third query.

**Affects:** measured 2026-09-17 -- aborts on `16.1.0`, `16.2.0` and trunk
`17.0.0 20260914` (`b76fde4b175`), and did on this branch until the fix below.
`13.4.0`, `14.4.0` and `15.3.0` do not accept the `post` syntax at all.  Clang
rejects the same programs cleanly.

## The bug

```c++
bool check (bool b) { return b; }
class S
{
  auto f ()
    post (r: check (r))
  { return e; }
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only postcondition-undeduced-result-ice.cpp
...: error: 'e' was not declared in this scope
...: internal compiler error: in check_noexcept_r, at cp/except.cc:1063
```

Four things are required together, and removing any one of them gives a clean
rejection:

| variation | result |
|---|---|
| as above | aborts |
| `bool f ()` -- concrete return type | clean |
| `post (true)` -- no result name | clean |
| `post (r: r)` -- no call in the predicate | clean |
| `{ return true; }` -- deduction succeeds | clean, accepted |

The failing deduction need not be a lookup failure, and the reported shape is
one of eight.  Each of these aborted, and each fails deduction for a different
reason:

```c++
bool g ();
auto f () post (r: g ()) { return f (); }       // use before deduction
auto f () post (r: g ()) { }                    // no return statement at all
auto *f () post (r: g ()) { }                   // auto* cannot deduce to void
decltype (auto) f () post (r: g ()) { return e; }
template <class T> auto f (T) post (r: g ()) { return e; }   // instantiated
auto l = [] () post (r: g ()) { return e; };
auto f () post (r: g ()) post (r2: true) { return e; }
```

So the trigger is "the return type was never deduced", not "the body contained
an undeclared identifier".

The no-return-statement row is the one that says most about where the defect
lives.  There, the right diagnostic *is* produced --

```
error: function does not return a value to test
```

-- by `rebuild_postconditions`, once `finish_function`'s auto-to-void fallback
has applied; and the compiler aborts anyway, after saying the correct thing.
That fallback invalidates the contract, so invalidation is demonstrably not
enough: by the time it runs, `maybe_apply_function_contracts` has already
spliced the checks into the body, and genericization walks them regardless.

## Every released compiler has this; only a checking build says so

The report describes an ICE on trunk, and a first pass at reproducing it
suggests trunk is the only affected version -- `16.1.0`, `16.2.0` and this
branch all print

```
...: confused by earlier errors, bailing out
```

instead.  That is the same abort.  `diagnostics/context.cc:1462-1477` converts
an ICE into a fatal error when the compiler was built without checking and an
error has already been emitted:

```c
      /* When not checking, ICEs are converted to fatal errors when an
	 error has already occurred.  This is counteracted by
	 abort_on_error.  */
      if (!CHECKING_P
	  && (diagnostic_count (kind::error) > 0
	      || diagnostic_count (kind::sorry) > 0)
	  && !m_abort_on_error)
```

The reporter's compiler is a checking build; the Compiler Explorer release
binaries and our own Release install are not.  Since the defect needs a prior
error to reach at all, that masking applies to every instance of it.

**The masked line is itself the proof, and is the only proof needed.**  That
branch of `report_diagnostic` is inside `if (m_kind == ice || m_kind ==
ice_nobt)` and is reachable from nowhere else, so a compiler that prints
`confused by earlier errors, bailing out` has had an internal error, full
stop.  An ordinary rejected program never prints it.

`-dH` looks like the way to unmask it and is not: it sets `abort_on_error`,
which aborts on **any** error, so it reports an abort whether or not this bug
is present -- `int main () { return e; }` with no contracts anywhere aborts
under `-dH` on the fixed compiler.  Anything built on it measures nothing.
A `-dH` row in `verify-cases.txt` and a `dg-ice` watch test built on it were
both tried, and both would have reported success against a compiler that
still had the bug.

What the harness needed instead was to learn the masked spelling:
`verify.sh`'s `_classify` now reads `confused by earlier errors` as `ice`.
Before that, the same defect read `ice` on the stock nightly and `error` on
our Release branch compiler, and the branch column read `error` both before
and after the fix -- so the row could never report its own repair.

## Analysis

The predicate of a postcondition with a result name is parsed as a template
tree and only made concrete later, and the "later" never arrives when
deduction fails.

`cp_parser_function_contract_specifier` (`gcc/cp/parser.cc`) raises
`processing_template_decl` around the predicate whenever a result identifier
is present:

```c
      if (identifier)
	{
	  /* Build a fake variable for the result identifier.  */
	  result = make_postcondition_variable (identifier);
	  ++processing_template_decl;
	}
      cp_expr condition = cp_parser_conditional_expression (parser);
```

It has to: the result variable's type is `make_auto ()` at that point, so the
predicate is genuinely dependent.  The consequence is that a call in the
predicate is built in template form, with an unresolved callee rather than a
pointer-to-function.

`rebuild_postconditions` (`gcc/cp/contracts.cc`) is what substitutes that tree
once the type is known, and it declines while the type is still undeduced:

```c
  /* If the return type is undeduced, defer until later.  */
  if (type_uses_auto (type))
    return;
```

"Later" is `apply_deduced_return_type`, which runs only when deduction
succeeds.  When the body fails to produce a type, `finish_function`'s
`auto`-to-`void` fallback in `finish_function` (`gcc/cp/decl.cc`, "If there
are no return statements in a function with auto return type") does not apply
either -- it is guarded on `!current_function_returns_value`, and a `return`
with an operand was seen.  The function therefore reaches genericization with its
return type still `auto` and its predicate still unsubstituted.

`cp_genericize_r` then calls `build_contract_check`, which asks
`expr_noexcept_p` whether the predicate can throw, and `check_noexcept_r`
walks into the template-form call:

```c
      tree fn = cp_get_callee (t);
      tree type = TREE_TYPE (fn);
      gcc_assert (INDIRECT_TYPE_P (type));
```

The callee has no pointer type, and the assertion fails.  That also explains
why the predicate must contain a call: `check_noexcept_r` asserts on nothing
else, so an unsubstituted predicate without one walks through harmlessly and
the same broken state produces no symptom.

This branch has the same two pieces in the same relationship
(`contracts.cc:4897`, `parser.cc:34034`), which is why it reproduces here
unchanged.  Nothing about the trigger depends on an extension of ours: plain
`-fcontracts` reaches it.

## Versions -- all on x86_64-linux-gnu

```
  source              version                       result
  compiler-explorer   13.4.0                        post syntax not supported
  compiler-explorer   14.4.0                        post syntax not supported
  compiler-explorer   15.3.0                        post syntax not supported
  compiler-explorer   16.1.0                        aborts (masked)
  compiler-explorer   16.2.0                        aborts (masked)
  compiler-explorer   17.0.0 20260914, b76fde4b175  ICE in check_noexcept_r
  this branch         17.0.0 20260914               aborts (masked), until fixed
```

"masked" is the `confused by earlier errors, bailing out` spelling described
above, which only the ICE path prints.  The one unmasked row is the only
checking build in the set.

Clang (this fork's contracts branch) diagnoses the same program cleanly and
does not crash.

## Discovery

Not ours.  It arrived as PR127450 from a fuzzer on the day it was filed and
was triaged into this directory: checked against the existing rows for a
duplicate, then measured across the release set and against this branch.

## Our Fix

`cp_genericize_r` now asks `contract_predicate_unsubstituted_p` before
building a check, and falls through to the `void_node` replacement it already
performs for a contract that produced none.
The test is the result variable's own type: `rebuild_postconditions` replaces
`POSTCONDITION_IDENTIFIER` with a copy carrying the deduced type in the same
breath as it substitutes the condition, so a result variable whose type still
uses `auto` is exactly one whose predicate was never substituted.  A
`gcc_checking_assert (seen_error ())` guards the one outcome that would be
worse than the abort -- silently dropping a check in a well-formed program.

The alternative considered was invalidating the contracts in `finish_function`
between the auto-to-void fallback and `cp_genericize`, reusing the existing
`error_mark_node` path.  It works, but it has to be placed in exactly that
window, and the no-return-statement row above is a live demonstration of what
happens when invalidation lands a moment too late.  Declining at the point of
emission has no ordering dependency and covers every route in, including the
outlined-check and P3595 dynamic-dispatch modes this branch adds.

The guard is in the caller rather than inside `build_contract_check` for a
reason worth recording: that `case POSTCONDITION_STMT` arm is upstream's byte
for byte, while this branch has rewritten `build_contract_check` entirely for
P3595.  A guard written inside the rewrite would be claimed by the P3595
entry in `branch-history/`, which lands after this one, so the history would
define the helper in one commit and call it several commits later -- and it
would read, to any reviewer, as a fragment of a rewrite rather than as a fix.

**Upstream is a separate question, and this code does not answer it.**  The
defect is upstream's and PR127450 is open, but the content of anything sent
to gcc-patches is the user's to write.  Nothing here is a proposed patch.

## Reproducer

See [`postcondition-undeduced-result-ice.cpp`](postcondition-undeduced-result-ice.cpp)
in this directory.  No headers, so no `.ii` is needed.  It carries the shape
PR127450 reports; the other seven are in the testsuite file below, which can
hold them all because the compiler no longer stops at the first.

## Notes

The regression test is
`gcc/testsuite/g++.dg/contracts/cpp26/postcondition-undeduced-result.C`, which
covers all eight shapes plus three controls -- deduction succeeding, a
postcondition with no result name on a function whose return type is still
undeduced when the checks are spliced, and a concrete return type.  The middle
one is the reason the fix cannot simply key off "the return type is `auto`
here".  The Clang mirror is
`clang/test/Contracts/OpenBugs/postcondition-undeduced-result.cpp`, which is
not xfailed: it pins Clang's clean rejection.

Worth adding to PR127450 if anyone comments on it: that the bug is not
trunk-only but reaches every release back to 16.1.0, and why it looks
trunk-only.  A reporter who tries 16.2.0 and sees `confused by earlier
errors` will read that as "already fixed in the release branch".
