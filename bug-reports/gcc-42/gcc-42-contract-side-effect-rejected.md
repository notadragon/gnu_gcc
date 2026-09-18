# GCC-42: A contract predicate that modifies the enclosing constant evaluation is rejected as non-constant

**Status:** Open (upstream); fixed here
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found.  Searched 2026-09-11 (`product=gcc`,
`component=c++`, summary `contract condition is not constant`), which
returns exactly three PRs, none of them this defect:
[PR125459](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125459) and
[PR125587](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125587) are
**GCC-3** -- see Notes for why they are a different bug behind the same
diagnostic -- and
[PR124100](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124100) is RESOLVED
and no longer reproduces.
**Affects:** re-measured 2026-09-17 -- rejected on `16.1.0`, `16.2.0` and
trunk `17.0.0 20260914` (`b76fde4b175`), in all seven predicate shapes on
trunk and fewer on the releases (see VERSIONS);
accepted, with a warning, on this branch.  Before
[`7b60a368fb1`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=7b60a368fb19b33f8676482a233d389cbc47b85e)
landed that day, four of the seven were accepted by accident -- see the side
note below.
**Resolved by:** [d43eec80f0c0](https://github.com/notadragon/gnu_gcc/commit/d43eec80f0c0efd5c12911f6d7a68225aa09948b)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `17.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a contract predicate that modifies the enclosing constant evaluation is rejected as "contract condition is not constant"` |

Attachments:

| File | Description |
|---|---|
| [`contract-side-effect-rejected.cpp`](contract-side-effect-rejected.cpp) | Three cases: the standard's own example, a member-function variant, and the runtime/constant-evaluation divergence |

The reproducer uses no headers, so it is also its own preprocessed source.

````
A contract predicate whose evaluation modifies an object of the enclosing
constant evaluation is rejected:

  error: contract condition is not constant

The predicate is a core constant expression and the program is well formed,
so this rejects valid code.  [basic.contract.eval] contains this exact
construct as a worked example:

  constexpr int f(int i)
  {
    contract_assert((++const_cast<int&>(i), true));
    return i;
  }
  inline void g()
  {
    int a[f(1)];  // size dependent on the evaluation semantic of
                  // contract_assert above
  }

The standard's comment is the whole point: the array's size DEPENDS ON the
evaluation semantic, which presupposes that under a checking semantic the
modification happens and is visible to the rest of the constant evaluation.
The note exists to warn that this makes the program ODR-sensitive -- not to
make it ill-formed.  g++ rejects it.

The const_cast is not incidental.  [basic.contract.general] const-ifies an
id-expression naming a variable declared outside the predicate; that is a
property of the expression, not of the object, so casting it away and then
modifying a non-const object is well defined.  Without the cast the code does
not compile for an unrelated reason, so any reproducer of this defect must
contain one.

Runtime and constant evaluation disagree about the same function:

  constexpr int h ()
  {
    string s ("foobar");                                   // i == 0
    contract_assert (const_cast<string &> (s).length () > 0);
    return s.i;
  }

  int main () { return h (); }   // runtime: exit status 1
  static_assert (h () == 1);     // error: contract condition is not constant

Measured on trunk 17.0.0 20260909: the runtime build returns 1, so the
predicate is evaluated and the side effect is real; the constant evaluation
of the identical function is rejected.  Clang accepts the standard's example.

DISCOVERY

Found while working through [basic.contract.eval]'s worked examples against
the implementation, one construct at a time.  This one is the paragraph's own
example, copied verbatim, so it was not a corner that had to be hunted for --
it is the example the standard chose to explain the feature with.

The same audit produced two neighbours in the same machinery: an [[assume]]
operand's refusal escaping as a diagnostic, and a nested assumption's side
effects surviving a discarded evaluation (filed as PR127282).


ANALYSIS

Contract predicates are evaluated under `modifiable_tracker`
(gcc/cp/constexpr.cc, the ASSERTION_STMT/PRECONDITION_STMT/POSTCONDITION_STMT
case).  That class exists for `[[assume]]`, whose operand is NOT evaluated
([dcl.attr.assume]) and whose speculative evaluation must therefore leave no
trace.  A contract predicate under a checking semantic IS evaluated, and the
tracked evaluation is the only one performed -- there is no second, real
pass.  So `constexpr_global_ctx::get_value_ptr` refuses the store, the
refusal is indistinguishable from "not a constant expression", and
`check_for_failed_contracts` reports it under [basic.contract.eval]/7.3.

The diagnostic machinery is correct; the input to it is not.  The tracker
cannot tell "this subexpression modified something outside itself" apart
from "this subexpression is not constant", and only the second is a reason
to reject.

This is not PR125459/PR125587.  Those share the diagnostic but have a
different cause -- a constexpr function re-called inside a contract
condition finds its own retired RESULT_DECL still present in the value map
and is refused permission to write it.  No modification of the enclosing
evaluation is involved there, and the fix is in how membership of the
modifiable set is decided, not in whether a refusal should be fatal.


VERSIONS -- all on x86_64-linux-gnu, measured 2026-09-17

  source              version                       rejects  errors on the reproducer
  compiler-explorer   16.1.0                        yes      2
  compiler-explorer   16.2.0                        yes      2
  compiler-explorer   17.0.0 20260914, b76fde4b175  yes      3

The error count RISES on trunk, and that is not a second defect.  Before
7b60a368fb1 (2026-09-14) a nested [[assume]] leaked its operand's side
effects, which happened to leave some of these predicates' modifications in
place and so masked the rejection.  Fixing that leak removed the accident and
exposed more of this defect -- so a reader comparing 16.2 against trunk sees
a regression that is really a mask being lifted.

13.4.0 does not accept -std=c++26; 14.4.0 and 15.3.0 accept the option but
not `contract_assert`, so the reproducer cannot be built before 16.1.0 and
the defect is as old as the syntax.
````

## Our Fix

Distinguish the two outcomes rather than removing the tracker.  Record, on
the refusal path, that a store was refused and to which object.  When a
contract predicate's evaluation fails and a store was refused, re-run the
predicate without the tracker and let the modification stand -- the program
is well formed and required to succeed.  Because the result then depends on
which evaluation semantic was chosen (under `ignore` the predicate is not
evaluated at all), warn rather than error, which is what the standard's ODR
note is actually cautioning about.

Kept out of the report block deliberately.  The analysis there says where the
defect is; what to do about it is upstream's call, and a report that
prescribes a patch invites an argument about the patch instead of agreement
about the bug.

## Notes

This branch implements the suggested fix in [d43eec80f0c0](https://github.com/notadragon/gnu_gcc/commit/d43eec80f0c0efd5c12911f6d7a68225aa09948b),
including the warning, spelled `-Wcontract-constexpr-side-effect`:

```
warning: contract predicate modifies 'i', an object of the enclosing
         constant evaluation [-Wcontract-constexpr-side-effect]
note: the predicate is not evaluated under the 'ignore' semantic, so the
      modification depends on the evaluation semantic
```

With it, all three cases in the reproducer compile and the runtime and
constant-evaluated answers agree.

The warning is on by default (`Init(1)`).  Upstream may prefer it off, or
prefer a different name; the correctness half stands without it.

**Distinct from GCC-3**, which shares the diagnostic text and the same
`modifiable_tracker` machinery but not the cause.  GCC-3 is about membership
of the modifiable set: a re-called constexpr function's retired `RESULT_DECL`
is still a key in the value map, so the write is refused
([445f896f0f64](https://github.com/notadragon/gnu_gcc/commit/445f896f0f6438cd64845b99ba9abc55c155399d) decides membership by liveness instead).
GCC-42 is about what a refusal *means*: even a correctly refused store to an
object of the enclosing evaluation does not make the predicate non-constant.
Both PRs filed against GCC-3 stop reproducing here, but by
[445f896f0f64](https://github.com/notadragon/gnu_gcc/commit/445f896f0f6438cd64845b99ba9abc55c155399d), not by the fix recorded on this row.

Measured, not merely argued: `-Wcontract-constexpr-side-effect` fires only
when the tracker actually refused a store and the predicate was then
re-evaluated, so it marks which fix a case needs.  On this branch the GCC-3
reproducer compiles with **no** such warning -- its store is never refused,
because [445f896f0f64](https://github.com/notadragon/gnu_gcc/commit/445f896f0f6438cd64845b99ba9abc55c155399d) corrected the membership test before
it got that far -- while the GCC-42 reproducer compiles with **two**.  So
the liveness fix does not resolve GCC-42, and the re-evaluation fix is not
what resolves GCC-3.

## Side note: this used to be masked, arbitrarily, by GCC-9

The two defects are independent, but they interacted, and the interaction is
worth recording because until 2026-09-14 it made upstream's behaviour look
arbitrary rather than merely wrong.

`modifiable_tracker` is constructed for an `[[assume]]` operand *and* for
every contract assertion, so the two nest.  While `~modifiable_tracker`
cleared `global->modifiable` outright (GCC-9,
[PR127282](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127282)), an inner
assertion switched the *enclosing* tracker off for the remainder of its
evaluation.  On a contract predicate that means the store this row is about
was no longer refused -- so the predicate was accepted, and upstream landed
on the standard-conforming answer by accident.

**That masking is gone.**  GCC-9 was fixed upstream on 2026-09-14 by
[`7b60a368fb1`](https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=7b60a368fb19b33f8676482a233d389cbc47b85e),
and this row is now what upstream does uniformly.  Varying only what precedes
the modifying subexpression in one contract predicate:

| inner construct | inner predicate | stock, before `7b60a368fb1` | stock, after |
|---|---|---|---|
| none | -- | **rejected** | **rejected** |
| free function, no argument | `true` | **rejected** | **rejected** |
| free function, `&s` argument | `true` | accepted, `i == 1` | **rejected** |
| free function, argument read from `s` | `p != nullptr` | accepted, `i == 1` | **rejected** |
| free function, literal argument | `p != nullptr` | **rejected** | **rejected** |
| lambda, no capture | `true` | accepted, `i == 1` | **rejected** |
| lambda, `[&]` capture | `true` | accepted, `i == 1` | **rejected** |

Before column measured on `gcc-16.2.0` and on trunk `17.0.0 20260909`; after
column on trunk `17.0.0 20260914` built at `b0730e1d39f`.  So the shape
dependence is gone and every row now hits this defect, which is the point:
fixing GCC-9 did not fix this, it stopped hiding it.

The mechanism was **nesting, and only nesting**.  Measured before the fix, on
stock `relwithdebinfo` trunk with `constexpr.cc` rebuilt at `-O0` so the
tracker's `global` resolves, breaking on the constructor and printing the
enclosing `global->modifiable`:

| variant | tracker constructed while another is live | outcome |
|---|---|---|
| no inner assertion | 0 | rejected |
| callee, `assert(true)` | 0 | rejected |
| callee, `assert(p != nullptr)` | **1** | accepted |
| callee, literal argument | 0 | rejected |
| lambda | **1** | accepted |

The correlation was exact: an inner tracker clobbered the outer if and only if
it was constructed while the outer was still live.  Counting constructions is
not enough and is what made this look arbitrary -- `callee, assert(true)`
constructs *two* trackers, but sequentially:

```
CTOR  enclosing modifiable = (nil)      <- the callee's assertion, on its own
DTOR  restoring to nullptr
CTOR  enclosing modifiable = (nil)      <- the outer predicate, afterwards
DTOR  restoring to nullptr
```

while the clobbering shape genuinely nests:

```
CTOR  enclosing modifiable = (nil)          <- outer predicate
CTOR  enclosing modifiable = 0x7fffffffc510 <- inner assertion, INSIDE it
DTOR  restoring to nullptr                  <- and clears the outer's set
DTOR  restoring to nullptr (was (nil))      <- outer now ungated
```

What decides which shape you get is whether the inner call is evaluated
*before* the outer's tracked evaluation begins or *during* it.  A call that
can be evaluated on its own -- no arguments, or arguments that are already
constants -- is evaluated in an earlier pass and its result is available
without re-entering the body, so its tracker has come and gone by then.  A
call whose argument depends on an object of the enclosing evaluation
(`ok1 (s.p)`), or a lambda whose closure is materialized during the
evaluation, cannot be, so its assertion runs inside the tracked region.

That is why the defect is invisible in small examples and appears in real
code: any contract assertion reachable from a predicate or an `[[assume]]`
operand whose arguments are not already constants will nest.

What the counts do establish is that these variants exercise measurably
different amounts of the machinery, which is why the test below keeps all of
them rather than the one shape that is understood.

None of this is a separate defect.  Fixing GCC-9 alone makes the behaviour
consistently *wrong* -- every row rejected, which is now measured rather than
predicted; fixing this row alone would make it consistently right for
contracts but leave `[[assume]]` corrupting its own operand.  Only both
together give the uniform behaviour this branch has: all seven rows accepted,
`i == 1`, one `-Wcontract-constexpr-side-effect` each.

In the other direction the same nesting was a wrong-code bug rather than a
rejection, because an `[[assume]]` operand genuinely must not be evaluated.
A contract assertion reached while evaluating one silently left the operand's
side effects in place:

```
constexpr bool with_assert (unsigned *) { contract_assert (true); return true; }
constexpr unsigned nested ()
{
  unsigned x = 0;
  [[assume (with_assert (&x) && bump (&x))]];   // operand is NOT evaluated
  return x;                                     // therefore 0
}
constexpr unsigned v = nested ();               // was 1.  correct: 0
char probe[v + 1];                              // and so sizeof differed
```

That half is **fixed**: `7b60a368fb1` makes trunk give 0, and `gcc-16.2.0`
still gives 1.  No diagnostic at `-Wall -Wextra -pedantic` on the releases
that have it; Clang gets 0 and warns under `-Wassume` that it is ignoring a
side-effecting assumption.  A `contract_assert (true)` in a called function
was enough -- the assumption did not have to contain a contract at all.

Coverage for both directions, across every shape in the table above, is
[`g++.dg/contracts/cpp26/contract-nested-modifiable-tracker.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/contract-nested-modifiable-tracker.C).
It is clean on this branch.

Discovered 2026-09-11 while reviewing the neighbouring nested-tracker fix
(GCC-9 / [PR127282](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127282)),
which touches the same class for an unrelated reason.  The two are
independent: GCC-9 was about trackers nesting, this is about whether a
tracker belongs on a contract predicate at all.
