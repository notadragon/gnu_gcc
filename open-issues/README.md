# Open Issues on This Branch

What is currently broken on the `contracts-p3850` branch of GCC -- whatever
its origin. If you hit something while using this compiler, look here first:
a row means we already know, and tells you whether there is a way around it.

A row is removed, and any file it points to in this directory deleted, as soon
as the issue is **fixed on this branch**. Git history is the record; nothing
is archived in place.

This is a different question from the one
[`../bug-reports/`](../bug-reports/README.md) answers. That directory tracks
bugs that reproduce on **stock upstream** GCC, including ones we have already
fixed here, and a row there survives until *upstream* fixes it. An upstream
bug we have not yet fixed appears in both; its single writeup lives in
`bug-reports/` and the row below links to it.

**Next ID:** GCC-34

IDs come from one sequence per compiler, shared with `bug-reports/`, and are
never reused. Allocate from the line above and increment it. Both tables
delete rows, so the highest ID visible in either is not a reliable counter.

`--` in the Upstream column would mean the issue is ours alone with nothing
upstream to link. **No row here qualifies**: contracts are upstream in GCC, so
every one of these is filable. (On the Clang side it does apply, to
contracts-dependent issues.) `None found (searched <date>)` means Bugzilla was
searched and nothing matched; `UNKNOWN` means nobody has looked.

**Kind** is one of `defect` (wrong, and we intend to fix it), `deferred`
(known, currently out of scope) or `divergence` (GCC and Clang disagree and
the standard does not clearly settle which is right).

**A new issue is always recorded as `defect` when it is discovered.** It
becomes `deferred` only when the user has explicitly said they do not want to
expand scope far enough to fix it -- never by an agent's own judgement that a
fix looks hard or invasive.

| ID | Symptom | Kind | Upstream | Details |
|----|---------|------|----------|---------|
| GCC-2 | Constant evaluation accepts forming the address of a member, or of a virtual base, before its constructor begins | deferred | [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (partial) | [../bug-reports/gcc-02-constexpr-vbase-before-ctor.md](../bug-reports/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-17 | `this` is accepted in the trailing return type of an explicit-object member function, where it is ill-formed | deferred | None found (searched 2026-09-05) | [../bug-reports/gcc-17-this-in-xobj-declaration.md](../bug-reports/gcc-17-this-in-xobj-declaration.md) |
| GCC-27 | Two friend declarations of one function with contradictory contracts are accepted silently | deferred | None found (searched 2026-09-05) | [../bug-reports/gcc-27-deferred-friend-contract-mismatch.md](../bug-reports/gcc-27-deferred-friend-contract-mismatch.md) |
