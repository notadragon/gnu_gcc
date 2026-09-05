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

**Next ID:** GCC-28

IDs come from one sequence per compiler, shared with `bug-reports/`, and are
never reused. Allocate from the line above and increment it. Both tables
delete rows, so the highest ID visible in either is not a reliable counter.

**Kind** is one of `defect` (wrong, and we intend to fix it), `deferred`
(known, deliberately not fixed yet) or `divergence` (GCC and Clang disagree
and the standard does not clearly settle which is right).

| ID | Symptom | Kind | Workaround | Upstream | Details |
|----|---------|------|------------|----------|---------|
| GCC-2 | Constant evaluation accepts forming the address of a member, or of a virtual base, before its constructor begins | defect | none; the program is accepted silently | UNKNOWN | [../bug-reports/gcc-02-constexpr-vbase-before-ctor.md](../bug-reports/gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-17 | `this` is accepted in the trailing return type of an explicit-object member function, where it is ill-formed | defect | name the object parameter instead, `decltype (self.x)` | UNKNOWN | [../bug-reports/gcc-17-this-in-xobj-declaration.md](../bug-reports/gcc-17-this-in-xobj-declaration.md) |
| GCC-27 | Two friend declarations of one function with contradictory contracts are accepted silently | deferred | declare the function at namespace scope and befriend that declaration | -- | [gcc-27-deferred-friend-contract-mismatch.md](gcc-27-deferred-friend-contract-mismatch.md) |
