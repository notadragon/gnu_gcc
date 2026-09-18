# GCC-33: A class-type result binding passed to a function taking a reference ICEs at codegen

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

So a single postcondition suffices, and what matters is a **class-type**
result binding being bound to a reference parameter. That is worth stating
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
([d4e55ecc0031](https://github.com/notadragon/gnu_gcc/commit/d4e55ecc0031ca7c5f0c68228cc2276655e16f13), a result binding denoting two different
objects) and the sret result-binding changes.
Establishing it means bisecting the branch, which has not been done.

Stated plainly because a report claiming a fix should be able to name it. If
the PR is answered before that is settled, the honest form is "this no longer
reproduces on our contracts branch" plus the narrowed reproducer, not a patch
offer.

## Notes

Found by the 2026-09-05 correlation sweep over all 33 open contracts PRs; the
sweep's negative results are in [`OTHER.md`](../OTHER.md).
