# GCC-3: "contract condition is not constant" in a constexpr function

**Status:** Fixed here (commit `aeba77c0b85`)
**Component:** c++ / constexpr
**Upstream Link:** [PR125459](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125459) (also PR125587, same defect)
**Affects:** stock g++ trunk (17.0.0 20260901) reproduces (all four
contract sites in the shipped reproducer error); stock g++ 16.2.0 does not
reproduce -- the contract-condition `modifiable_tracker` path postdates
16.2.0 (see Notes)

## Bug Report

A contract condition is evaluated under a `modifiable_tracker`, so that a
predicate cannot modify objects belonging to the enclosing constant
evaluation. `constexpr_global_ctx::put_value` decides membership of the
"modifiable" set by whether the object is already a key in the value map,
while `destroy_value` retires a `VAR_`/`PARM_`/`RESULT_DECL` by leaving it
in that map mapped to `void_node` rather than removing it. So a constexpr
function called once, returned from, and then called again from inside a
contract condition (a `contract_assert`, a `pre`, or a `post`) finds its
own `RESULT_DECL` already present, is refused permission to write it, and
the condition is wrongly reported non-constant:

```
error: contract condition is not constant
```

under a terminating semantic, and the same text as a warning under
`observe`. This rejects a well-formed program: the re-called function has
nothing wrong with it, and the two calls are entirely independent. This is
PR125459 (and PR125587, filed independently against the same defect, with
`post` in the title): both are open upstream reports whose reporters hit
exactly this symptom and cite this same conceptual fix.

## Reproducer

See [`gcc-03-constexpr-repeat-call.C`](gcc-03-constexpr-repeat-call.C) in
this directory.

## Our Fix

`gcc/cp/constexpr.cc`, `constexpr_global_ctx::put_value`: decide
membership of the modifiable set by whether the object is currently alive,
instead of whether it is present as a map key at all. An object whose
lifetime has ended is a new object when given a value again and belongs to
the subexpression that creates it; a live object really was created
outside and stays unmodifiable. The defect is in code shared with the
other `modifiable_tracker` user, `cxx_eval_assert` (i.e. `[[assume]]`),
though it is invisible there because a non-constant assume result is
simply ignored rather than diagnosed.

## Notes

The primary reproducer, `gcc-03-constexpr-repeat-call.C`, was extracted
from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-repeat-call.C`
(added by gnu_gcc commit `aeba77c0b85`), found via
`git show aeba77c0b85 --stat` and read with
`git show aeba77c0b85:gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-repeat-call.C`.
DejaGnu directive lines were stripped, and the P3400 assertion-label test
functions (which need this branch's `-fcontracts-p3400` flag) were
dropped, since the commit message states the defect has "nothing to do
with contract labels."

Compiling that extracted reproducer against the two stock binaries
available (`/home/jberne4/repos/compilers/gcc-16.2.0/bin/g++` and
`gcc-trunk/bin/g++`, 17.0.0 20260901) with `-std=c++26 -fcontracts`
confirms the split in **Affects** above. On stock g++ trunk, all four
contract sites fail exactly as described:

```
gcc-03-constexpr-repeat-call.C:53:3: error: contract condition is not constant
   53 |   contract_assert (!v->empty ());
      |   ^~~~~~~~~~~~~~~
gcc-03-constexpr-repeat-call.C:61:20: error: contract condition is not constant
   61 | step_pre (view *v) pre (!v->empty ())
      |                    ^~~~~~~~~~~~~~~~~~
gcc-03-constexpr-repeat-call.C:71:27: error: contract condition is not constant
   71 | step_post (view *const v) post (r : r <= 1u && (v->empty () || !v->empty ()))
      |                           ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
gcc-03-constexpr-repeat-call.C:103:3: error: contract condition is not constant
  103 |   contract_assert (!outer_empty (v));
      |   ^~~~~~~~~~~~~~~
```

-- the `contract_assert` at line 53, the `pre` at line 61, the `post` at
line 71, and the nested `contract_assert` (via `outer_empty`) at line 103,
with `g++` exiting 1. On stock g++ 16.2.0, the identical command compiles
cleanly (exit 0) and every `static_assert` evaluates to its correct value:
16.2.0 does not yet route a contract condition's constant evaluation
through the `modifiable_tracker` path this defect lives in (that path was
already present for `[[assume]]`'s `cxx_eval_assert`, per the shared root
cause described above, but was not yet reached from `contract_assert`/
`pre`/`post`), so there is nothing there for the false rejection to occur
in.

Per this repository's internal correlation notes
(`notadragon_wg21`'s `src/pubs/impl/p3850impl/gcc-bugs/upstream-correlation.md`,
read-only), PR125459 and PR125587 are real, currently-open upstream
reports citing this fix commit; the trunk reproduction above matches their
described symptom directly.

This bug was originally excluded from an earlier internal migration pass
(see `pending_reports.md`'s old GCC-3 entry) on the reasoning that there
was no independently-reproducible symptom on unmodified GCC, since at the
time the only known `modifiable_tracker` consumer upstream was
`cxx_eval_assert` (`[[assume]]`), where a non-constant result is silently
ignored rather than diagnosed. That reasoning is now confirmed stale:
PR125459 and PR125587 are real, currently-open Bugzilla reports of users
hitting this exact symptom through contracts (not `[[assume]]`) on stock
GCC, citing this same fix, and the trunk reproduction above demonstrates
the symptom directly.
