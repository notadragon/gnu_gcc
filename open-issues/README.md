# Open Issues on This Branch

What is currently broken on the `contracts-p3850` branch of GCC -- whatever
its origin.  If you hit something while using this compiler, look here first:
a row means we already know, and tells you whether there is a way around it.

A row is removed, and any file it points to in this directory deleted, as
soon as the issue is **fixed on this branch**.  Git history is the record;
nothing is archived in place.

This is a different question from the one
[`../bug-reports/`](../bug-reports/README.md) answers.  That directory tracks
bugs that reproduce on **stock upstream** GCC, including ones already fixed
here, and a row there survives until *upstream* fixes it.  An upstream bug
not yet fixed here appears in both; its single writeup lives in
`bug-reports/` and the row below links to it.

## What the columns mean

**Kind** is one of `defect` (wrong, and we intend to fix it), `deferred`
(known, currently out of scope) or `divergence` (GCC and Clang disagree and
the standard does not clearly settle which is right).

**Upstream** distinguishes three states.  A link means a PR exists.
`None found (searched <date>)` means Bugzilla was searched and nothing
matched.  `UNKNOWN` means nobody has looked yet.  `--` means the issue is
ours alone with nothing upstream to link -- either because the trigger needs
one of this fork's own extensions, which are not upstream, or because a
regression lives in machinery shared with upstream but does not reproduce
there.  A trailing `*` marks a PR filed from this work.

**Details** links the writeup.  Most rows point into `bug-reports/`, because
most branch-only issues are also upstream bugs not yet fixed here; an issue
gets its own file in this directory only when it does not reproduce upstream
at all.

## The table

**Next ID:** GCC-44

| ID | Symptom | Kind | Upstream | Details |
|----|---------|------|----------|---------|
| GCC-2 | Constant evaluation accepts forming the address of a member, or of a virtual base, before its constructor begins | deferred | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (partial) | [../bug-reports/gcc-02/gcc-02-constexpr-vbase-before-ctor.md](../bug-reports/gcc-02/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-17 | `this` is accepted in the trailing return type of an explicit-object member function, where it is ill-formed | deferred | [PR127290](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127290) * | [../bug-reports/gcc-17/gcc-17-this-in-xobj-declaration.md](../bug-reports/gcc-17/gcc-17-this-in-xobj-declaration.md) |
| GCC-27 | A friend declaration's contract is never checked against an earlier declaration of the same function | deferred | [PR127291](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127291) * | [../bug-reports/gcc-27/gcc-27-deferred-friend-contract-mismatch.md](../bug-reports/gcc-27/gcc-27-deferred-friend-contract-mismatch.md) |
| GCC-39 | Explicit instantiation of a variadic template rejects a cv-qualified pack argument, so the instantiation is not emitted (not a contracts bug) | deferred | [PR126797](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126797) | [../bug-reports/gcc-39/gcc-39-explicit-inst-pack-cv.md](../bug-reports/gcc-39/gcc-39-explicit-inst-pack-cv.md) |
