# GCC-44: A violation handler throwing out of a postcondition leaks the returned object

**Status:** Fixed here
**Resolved by:** `1140-retval-destroyed-on-unwind`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** [PR127414](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127414)
-- **FILED 2026-09-15**, UNCONFIRMED. (Searched 2026-09-15 before filing:
`contracts postcondition` returns 19 bugs, none about a leak, and
`postcondition leak`, `contract_violation throw` and
`throwing violation handler` return nothing relevant.)
**Affects:** measured 2026-09-15 -- leaks on `16.1.0`, `16.2.0` and trunk
`17.0.0 20260914 (experimental)` (`b76fde4b175`). 13.4.0, 14.4.0 and 15.3.0
do not accept the syntax. Fixed on this branch (re-measured the same day).

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a violation handler throwing out of a postcondition leaks the returned object` |

Attachments:

| File | Description |
|---|---|
| [`postcondition-throw-leaks-retval.cpp`](postcondition-throw-leaks-retval.cpp) | Returned object leaked: exits 2, and 0 with the postconditions deleted |

````
A contract-violation handler that exits by throwing out of a postcondition
leaks the returned object.  The object has been initialized by the time the
postcondition is evaluated, and after unwinding the program can no longer
reach it, so nothing ever runs its destructor.

```
#include <contracts>

int live = 0;

struct Counted {
    Counted () { ++live; }
    Counted (const Counted &) { ++live; }
    ~Counted () { --live; }
};

struct E { };

void handle_contract_violation (const std::contracts::contract_violation &)
{
    throw E { };
}

Counted f (const int n) post (r : n > 100)
{
    // NRVO
    Counted result;
    return result;
}

Counted g (const int n) post (r : n > 100)
{
    // normal return (RVO)
    return Counted {};
}

int main ()
{
    try { f (1); } catch (E &) { }
    try { g (1); } catch (E &) { }
    return live;                 // 0 expected; 2 as it stands
}
```

```
$ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce \
      postcondition-throw-leaks-retval.cpp -lstdc++exp && ./a.out; echo $?
2
```

Without the postconditions, or with a throw in the body before we reach
postconditions, the counted objects are all destroyed properly.

The wording in [except.ctor]/2 that should demand that the return object's
destructor be run during unwinding is questionable, so there is a CWG issue
here (that has been submitted).  Even without that CWG issue having been
approved, it's unquestionable that we don't want to leak resources in this
situation and the return value must be destroyed when we unwind out of the
violation handler.


DISCOVERY

Found while migrating uses of BSLS_ASSERT to pre/post in the BDE libraries,
in the same sweep that turned up the double-destroy defect in PR127281.  Both
concern the return object's cleanup around the artificial block that carries
the contract checks, from opposite directions: there the block splices a
second cleanup, here nothing covers the checks at all.


ANALYSIS

maybe_apply_function_contracts (gcc/cp/contracts.cc) builds

  TRY_FINALLY_EXPR
    op 0: the user's body
    op 1: EH_ELSE_EXPR
            op 0: apply_postconditions ()   <- normal-completion arm
            op 1: void_node                 <- exceptional arm, checks skipped

so the postcondition checks live in the finally, not in the body.  The only
cleanup that destroys DECL_RESULT is the one maybe_splice_retval_cleanup
(gcc/cp/except.cc) splices around the function body, guarded by
current_retval_sentinel.  That cleanup covers op 0 and stops there; nothing
covers op 1.  An exception leaving the checks -- which is exactly what a
violation handler that throws produces -- therefore unwinds past a result
object with no cleanup attached, and the object is never destroyed.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       leaks
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260914, b76fde4b175  yes
  local build -g      17.0.0 20260909, 7dab38c9d71  yes
````

## Reproducer

See [`postcondition-throw-leaks-retval.cpp`](postcondition-throw-leaks-retval.cpp)
in this directory.  It has an `#include <contracts>`, so a `.ii` should
accompany any Bugzilla attachment.

## Our Fix

`gcc/cp/contracts.cc`: a new static helper
`wrap_postconditions_in_retval_cleanup` wraps the postcondition checks -- and
only them -- in an EH-only `CLEANUP_STMT` that destroys the returned object,
applied at both `apply_postconditions` call sites (the `EH_ELSE`
non-exceptional path and the plain path).

It deliberately carries no sentinel guard, unlike `maybe_splice_retval_cleanup`:
the region is reached only on the normal-completion path of the body, where
the returned object necessarily exists.  That is also what keeps the two
cleanups from overlapping -- the body's covers the body and stops there, this
one covers only the checks -- so the object is destroyed exactly once however
the function unwinds.  That non-overlap is the whole correctness argument,
and is what a reviewer should check first.

Tests: `contract-retval-destroyed-on-unwind.C` (counts constructions against
destructions, so a leak and a double destroy both fail),
`postcondition-throw-escapes-try.C`,
`postcondition-throw-noexcept-terminate.C`, and
`open-bug-retval-throwing-cleanup.C`.

## Notes

This and GCC-5 are now fixed together by
`1140-retval-destroyed-on-unwind`.  There is no longer a non-overlap argument
to make for ordinary functions: the single cleanup deferred to the contracts
block covers the body and the postcondition checks, so there is only one.  A
coroutine ramp still uses the separate
`wrap_postconditions_in_retval_cleanup`, because its transform clears
`throwing_cleanup` and leaves no sentinel to defer onto; there the two
mechanisms are selected exclusively, never combined.

Upstream the two still want to go as a series in PR127281, PR127414 order --
upstream has no per-assertion semantics, so it must force the sentinel on for
every postcondition rather than only the ones that can throw.

The wording does not currently require the destruction, and that is a defect
in the wording rather than a licence to leak.  [except.ctor]/2
destroys the returned object only for an exception thrown "during the
destruction of temporaries or local variables for a return statement" and
says nothing about contract assertions; [basic.contract.eval] says a throwing
handler behaves "as if the function body exits via that same exception",
describing a state in which the result object was never initialized -- which
is not the state the program is actually in. A core issue has been filed.
Leaking an object the program can no longer reach is not a defensible
reading, and the tests say so explicitly: they must not be "corrected" back
to expecting a leak on the strength of the wording as it stands.
