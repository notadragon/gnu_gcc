---
id: 9900-fork-metadata
subject: 'Fork metadata: README, bug reports, open issues, branch history'
depends: []
regenerates: []
fixes: []
---

## Rationale

Everything about this branch that is *about* the work rather than part of
it, gathered into the last commit so that dropping one commit leaves a
purely upstreamable series.

Four things live here.  `README` gains a prepended description of the fork,
the papers it implements and how to enable them -- a pure prepend, so
dropping this commit restores upstream's `README` byte-for-byte.
`bug-reports/` holds the bugs that reproduce on **stock upstream GCC**, one
directory per bug with notes and reproducers, and a table naming the commit
in this very branch that resolves each one.  `open-issues/` holds what is
still broken *here*, whatever its origin.  `branch-history/` is the mapping
that generates this history, including the file you are reading.

**A reviewer should read none of this as compiler work.**  It is never
upstreamed and is deliberately isolated: no commit before this one may touch
any of these paths, and `check` enforces that as the clean-drop invariant.

Two properties are worth knowing.  This commit is **generated last** so that
it can point backwards: the hash column in `bug-reports/README.md` is filled
in from the commits that precede it, which is why those references are
written as stable pseudo-commit ids everywhere else and resolved to real
hashes only here.  And it claims its paths **by glob rather than by
enumeration**, because it owns `branch-history/` itself -- otherwise every
re-seed of the mapping would have to re-list the files the previous seed
created.

## Compile gap

None.  Nothing here is compiled, and nothing in the compiler refers to it.

The one property that must hold is negative rather than constructive: this
commit must remain the *only* one touching `README`, `bug-reports/`,
`open-issues/` and `branch-history/`.  If an earlier commit ever claims one
of those paths, dropping this commit stops yielding a clean upstreamable
series -- so the constraint is machine-checked rather than left to habit.

## Contents

- README : *
- bug-reports/* : *
- open-issues/* : *
- branch-history/* : *
