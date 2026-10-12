# GCC-16: Contract on a lambda that captures, inside a template, ICEs

**Status:** Fixed here (commits [d9254c1ad651](https://github.com/notadragon/gnu_gcc/commit/d9254c1ad6518bad44c534a25eb836ef6322e0a9)
and [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb))
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
(added by [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb)).
The DejaGnu `dg-do run`, `dg-additional-options`, and `dg-skip-if` directive
lines were stripped; the C++ source (including the test's own commentary
comment block) was kept as-is. Requires `-fcontracts
-fcontract-evaluation-semantic=observe`, linked with `-lstdc++exp`, and a
hosted `<contracts>` header (C++26) to build and run (the stripped
`dg-skip-if` noted this needs a hosted libstdc++ for `stdc++exp`).

## Pending upstream comment (PR126038)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR126038.  Every claim
re-measured 2026-09-25 against stock `17.0.0 20260922` (the Compiler
Explorer nightly) and 16.2.0.  The report has only a preprocessed
attachment, so the testcase below is reconstructed from its prose.  Attach
[d9254c1ad651](https://github.com/notadragon/gnu_gcc/commit/d9254c1ad6518bad44c534a25eb836ef6322e0a9), which is
self-contained.  The template shape in this writeup's title needs more
(the capture-proxy bridge in [7a1a3c080b75](https://github.com/notadragon/gnu_gcc/commit/7a1a3c080b7576882a11f7627f1e312bbefb51bb) and a
`process_outer_var_ref` carve-out) and is not claimed in this comment.*

~~~text
A plain-text reproducer, since the report only has the preprocessed
attachment:

  int f (int a) {
    int lo = 5;
    auto l = [lo] (int b) pre (b > lo) { return b; };
    return l (a);
  }

  internal compiler error: in gimplify_var_or_parm_decl, at gimplify.cc:3298

It is narrower than the description: only a precondition that names one of
the lambda's own explicit captures crashes.  By copy, by reference and an
init-capture all do; the same predicate as a postcondition, post (r : r >
lo), compiles and runs correctly.  Same on 16.2.0 and trunk (17.0.0
20260922).

The undeclared VAR_DECL is the capture proxy.  maybe_apply_function_contracts
hoists a lambda body's BIND_EXPR out and reopens it around the contracts,
so that the proxies stay in scope for the predicates, but it only matches a
body that is itself a BIND_EXPR.  Here the proxy-declaring BIND_EXPR
arrives wrapped in a STATEMENT_LIST holding nothing else, the hoist is
skipped, and the precondition is emitted before the bind that declares the
proxies.  Postconditions are emitted after the body, by which time the
gimplifier has seen that bind, which is why they are unaffected.

The fix looks through a single-statement STATEMENT_LIST when looking for
the BIND_EXPR.
~~~
