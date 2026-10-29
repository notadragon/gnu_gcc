# GCC-70: Constant evaluation accepts destroying an object twice (explicit destructor call, then `delete`)

**Kind:** defect (accepts-invalid)
**Status:** Open (upstream); fixed here 2026-09-30 -- found 2026-09-25
(review round #12, B-misc#14, from the celink out-of-scope drift check)
**Resolved by:** [10ef2099e1a5](https://github.com/notadragon/gnu_gcc/commit/10ef2099e1a52c3af7d7d10bc2abbd59887182a8)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** [PR126425](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126425)
(UNCONFIRMED, filed 2026-07-27: `std::destroy_at` then `delete` in a
`consteval` block).  Found by Bugzilla REST quicksearch 2026-09-25 on
`constexpr lifetime destructor delete`.
**Affects:** measured 2026-09-25 -- 14.4.0, 15.3.0, 16.1.0, 16.2.0, stock
trunk `17.0.0 20260922` and this branch all accept the reproducer.
**Workaround:** none; the code is undefined, so the fix is in the user's code
**Clang:** correct -- trunk and the Clang fork reject it with "destroying
object '{*new S#0}' whose lifetime has already ended"

## Bug Report

Already filed as PR126425, with `std::destroy_at` and reflection's
`consteval` block.  Nothing new to file; the useful contribution is a
confirming comment with this smaller reproducer (plain C++20, explicit
destructor call, no library, no `-freflection`), and the versions above.

| File | Description |
|---|---|
| [`constexpr-double-destroy.cpp`](constexpr-double-destroy.cpp) | `p->~S ()` then `delete p` in a `static_assert` |

## Symptom

`g++ -std=c++20 -fsyntax-only constexpr-double-destroy.cpp` is silent, exit
0.  Expected: "non-constant condition for static assertion", with the second
destruction as the reason.

## Notes (ours, not for the report)

GCC already tracks lifetime end for the explicit destructor call (a later
read of `p->f` is rejected, as an access to an uninitialized member), and
`destroy_value_checked` (`gcc/cp/constexpr.cc`) has a "destroying %qE outside
its lifetime" diagnostic, but the destructor call that the delete-expression
evaluates does not trigger it.

## Regression test

`g++.dg/contracts/cpp26/constexpr-heap-double-destroy.C`, which replaced the watch test when the
defect was fixed here.  The Clang mirror is
`clang/test/Contracts/constexpr-double-destroy.cpp`.
