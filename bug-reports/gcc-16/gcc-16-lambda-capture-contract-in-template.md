# GCC-16: Contract on a lambda that captures, inside a template, ICEs

**Status:** Fixed here (commits [ca12ab026d5d](https://github.com/notadragon/gnu_gcc/commit/ca12ab026d5d0e02bf7dbc86966e255733fd02d2)
and [445a6a73230a](https://github.com/notadragon/gnu_gcc/commit/445a6a73230a769c16f73013c309e3b61d23b705))
**Resolved by:** `1230-lambda-capture-bind-through-statement-list` (the
`STATEMENT_LIST` look-through in `maybe_apply_function_contracts`) for the
shape that needs no template, which is the one PR126038 itself reports;
`4640-contract-substitution-plumbing` (the two-hop capture-proxy bridge in
`pt.cc`) together with `4000-p3098` (the `process_outer_var_ref` carve-out)
for the shape inside an instantiated template, where neither half works alone
**Component:** c++ / contracts
**Upstream Link:** [PR126038](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126038) -- correlated by the 2026-09-05
sweep: a contract on a lambda capturing surrounding locals (by reference OR by
value) ICEs stock trunk in `gimplify_var_or_parm_decl` and compiles clean
here. The report supplies only a preprocessed attachment, so the shape was
reconstructed from its prose description; it reproduces. Note the PR's own
case needs no template, where this entry's title says one -- the template is
not load-bearing, and the report is the broader statement of the same defect.

**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

A contract predicate on a lambda that captures a local (by value or
reference) hits an internal compiler error (ICE) when the lambda is written
inside an instantiated
template, because nothing maps the pattern lambda's capture proxy to the
instantiation's, and `process_outer_var_ref` has no carve-out for a capture
proxy inside a contract condition. This reproduces on stock g++ 16.2.0 and
g++-trunk (17.0.0 20260901).

## Reproducer

See [`gcc-16-lambda-capture-contract-in-template.C`](gcc-16-lambda-capture-contract-in-template.C)
in this directory.

## Our Fix

`gcc/cp/contracts.cc`, `maybe_apply_function_contracts`: look through a
single-statement `STATEMENT_LIST` when locating the lambda body's `BIND_EXPR`,
so the capture proxies are in scope for the predicates.  That alone fixes the
shape that needs no template.

`gcc/cp/pt.cc` and `gcc/cp/semantics.cc`: bridge each capture proxy named in
a predicate to the instantiation's, via a two-hop `local_specializations`
lookup registered into the CURRENT map, before substituting contracts;
extend `process_outer_var_ref`'s parameter carve-out to capture proxies.
That is what the template shape additionally needs.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-contract-in-template.C`
(added by [445a6a73230a](https://github.com/notadragon/gnu_gcc/commit/445a6a73230a769c16f73013c309e3b61d23b705)).
The DejaGnu `dg-do run`, `dg-additional-options`, and `dg-skip-if` directive
lines were stripped; the C++ source (including the test's own commentary
comment block) was kept as-is. Requires `-fcontracts
-fcontract-evaluation-semantic=observe`, linked with `-lstdc++exp`, and a
hosted `<contracts>` header (C++26) to build and run (the stripped
`dg-skip-if` noted this needs a hosted libstdc++ for `stdc++exp`).
