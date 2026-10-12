# GCC-33: An empty-class result binding passed to a function taking a reference ICEs at codegen

**Status:** Fixed here (which change fixed it is not established -- see below)
**Resolved by:** `1130-result-binding-one-object` -- attributed rather than
established: that commit carries the result-binding work named below,
and pins `pr125574.C`, but no bisection has confirmed it
**Component:** c++ / contracts
**Upstream Link:** [PR125574](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125574)
**Affects:** measured 2026-09-05 -- reproduces on stock g++ trunk
(17.0.0 20260901) with plain `-fcontracts`

## Bug Report

```cpp
struct Vec { };
bool ok (const Vec &) { return true; }
Vec f () post (r : ok (r)) { return {}; }
```

```
internal compiler error: in expand_expr_addr_expr_1, at expr.cc:9355
```

**`-fsyntax-only` is not enough to show it** -- the ICE is past the front
end, so a syntax-only check reports nothing and the bug looks absent.

## Narrower than the report

PR125574 uses **two** postconditions and concludes both are needed ("Must be
Vec&, Vec compiles"). Measured here, neither claim is quite the boundary:

| shape | stock trunk |
|---|---|
| one `post (r : ok (r))`, class result, `const Vec &` parameter | **ICE** |
| one `post (r : true)`, class result, binding unused | fine |
| one `post (r : ok_int (r))`, **scalar** result, `const int &` parameter | fine |

So a single postcondition suffices, and what matters is an **empty-class**
result binding being bound to a reference parameter: give `Vec` a member
and stock trunk compiles it (checked 2026-09-29). That is worth stating
when the PR is answered, because it makes the reproducer half the size and
points at the result-binding materialisation rather than at postcondition
count.

## Reproducer

See [`gcc-33-class-result-binding-by-reference.cpp`](gcc-33-class-result-binding-by-reference.cpp)
in this directory, which carries all four shapes above.

## Our Fix

This branch compiles all four cleanly, but **which of our changes fixed it has
not been established** -- it was found by correlation, not by being diagnosed
and fixed here. The result-binding work is the obvious place to look: GCC-12
([80e027046385](https://github.com/notadragon/gnu_gcc/commit/80e027046385da7625faff1a74f334b1f76e3a81), a result binding denoting two different
objects) and the sret result-binding changes.
Establishing it means bisecting the branch, which has not been done.

Stated plainly because a report claiming a fix should be able to name it. If
the PR is answered before that is settled, the honest form is "this no longer
reproduces on our contracts branch" plus the narrowed reproducer, not a patch
offer.

## Notes

Found by the 2026-09-05 correlation sweep over all 33 open contracts PRs; the
sweep's negative results are in [`OTHER.md`](../OTHER.md).

## Pending upstream comment (PR125574)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR125574.  Every claim
re-measured 2026-09-25 against stock `17.0.0 20260922` (the Compiler
Explorer nightly) and 16.2.0.  Two corrections to this writeup came out of
the re-measurement and are reflected here: the trigger is an empty class
(a non-empty one, `struct Vec { int x; }` or `{ int x[16]; }`, compiles on
stock), and the fix is known -- `pr125574.C` and the comment on
`postcondition_needs_retval_temp_p` in
[80e027046385](https://github.com/notadragon/gnu_gcc/commit/80e027046385da7625faff1a74f334b1f76e3a81) name it.  waffl3x's comment 1 already
reduced the report to one postcondition, so this does not repeat that.
The draft in `upstream-notes.md` also proposed a note on PR110872; that PR
is RESOLVED FIXED, so the in-tree test is mentioned here instead.  Attach
[80e027046385](https://github.com/notadragon/gnu_gcc/commit/80e027046385da7625faff1a74f334b1f76e3a81) (the `postcondition_needs_retval_temp_p`
half; the rest of `apply_postconditions` there is entangled with P3098
captures).*

~~~text
Two more data points, and a fix.

The class has to be empty: with struct S { int x; }, or a larger aggregate,
the same postcondition compiles.  And the reference parameter is one way of
taking the result's address, not the only one; ok (&r) with a pointer
parameter ICEs the same way.  So the trigger is a postcondition that needs
the address of a result that is returned in registers.

The standard's own second example in [dcl.contract.res] hits it verbatim:

  struct A {};
  struct B { B() {} B(const B&) {} };

  template <typename T>
  T f(T* const ptr) post(r: &r == ptr) { return {}; }

  int main() {
    A a = f(&a);
    B b = f(&b);
  }

  internal compiler error: in expand_expr_addr_expr_1, at expr.cc:9354

Binding the result name to DECL_RESULT is the same move as the NRVO, but
without the NRVO's guard: want_nrvo_p only aliases when aggregate_value_p
says the result lives in memory.  For a result returned in a register,
DECL_RESULT gets a pseudo and there is no address to take.

The fix gives the checks a temporary instead, when the result is not
returned in memory, is trivially copyable, and the result binding has its
address taken: initialized from DECL_RESULT before the checks and copied
back after them, so a write through the binding is still seen by the
caller.  That is also what the example describes: the check "can fail if
the implementation introduces a temporary for the return value", which is
the register-returned A, and succeeds for B, which is returned in memory.

g++.dg/coroutines/pr110872.C has a dg-ice for this ICE; the coroutine in
it is incidental, and with the fix it can become the run test its NOTE
asks for.
~~~
