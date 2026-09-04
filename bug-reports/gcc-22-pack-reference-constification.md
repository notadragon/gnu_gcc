# GCC-22: A parameter pack of reference type in a contract is rejected outright

**Status:** Fixed here (commit `ea65bf85250`)
**Component:** c++ / contracts, constification
**Upstream Link:** [PR126878](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126878) (also PR126039, same root cause via a generic lambda's `auto&&...`)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

A function parameter pack of lvalue-reference type (`Args&... args`), or a
forwarding-reference pack (`Args&&... args`, including a generic lambda's
`auto&&... args`), named in a `pre` or `post` contract is rejected
outright:

```
error: 'const' qualifiers cannot be applied to 'Args&'
```

Contract access is constified by `view_as_const`, which cv-qualifies the
accessed type. A reference cannot be cv-qualified -- and never needed to
be here: a non-pack reference parameter arrives already dereferenced, as a
`REFERENCE_REF` whose type is the referent's, so constification reaches
the referent and the reference itself is never touched. A pack element,
however, is still a bare `PARM_DECL` of reference type at the point
constification runs, because the pack has not yet been expanded, so
`view_as_const` reaches the reference type directly and the hard error
rejects an otherwise well-formed program. This affects `pre` and `post`
alike, and pack-index-expressions as well as fold-expressions.

## Reproducer

See [`gcc-22-pack-reference-constification.C`](gcc-22-pack-reference-constification.C)
in this directory. On stock g++ 16.2.0 (and trunk):

```
$ g++ -std=c++26 -fcontracts gcc-22-pack-reference-constification.C -c
error: 'const' qualifiers cannot be applied to 'Args&'
...
error: 'const' qualifiers cannot be applied to 'Args&'
...: confused by earlier errors, bailing out
```

## Our Fix

`gcc/cp/contracts.cc`: new `contract_ref_or_ref_pack_p` predicate, used by
`view_as_const` to leave a reference, or a pack expansion of one,
unqualified. Testing the type with `TYPE_REF_P` alone is not enough,
because `cp_build_qualified_type` recurses through the pack expansion into
its pattern and the reference check fires there; the new predicate
recognizes both spellings. Leaving the pack alone is correct rather than
merely quiet: at instantiation each expanded element is an ordinary
reference parameter and is constified the usual way.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/pr126878.C` (added by gnu_gcc commit
`ea65bf85250`), found via `git show ea65bf85250 --stat` and read with
`git show ea65bf85250:gcc/testsuite/g++.dg/contracts/cpp26/pr126878.C`.
The DejaGnu `dg-do run`, `dg-additional-options`, and `dg-skip-if`
directive lines were stripped; the C++ source, including the test's own
commentary and the runtime violation-counting harness, was kept as-is.

Measured directly against the stock binaries at
`/home/jberne4/repos/compilers/gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`
(17.0.0 20260901): both reject `pre_pack (Args&... args) pre (...)` with
the exact `'const' qualifiers cannot be applied to 'Args&'` error at
`-std=c++26 -fcontracts`, confirming the defect on both. Older stock
compilers (13.4.0, 14.4.0, 15.3.0) do not support C++26 contracts syntax
at all and are not applicable.
