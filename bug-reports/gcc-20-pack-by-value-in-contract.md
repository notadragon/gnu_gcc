# GCC-20: A pack of non-reference type named in a contract ICEs template substitution

**Status:** Fixed here (commit `3eb1e3f1974`)
**Component:** c++ / contracts, template substitution
**Upstream Link:** [PR126804](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126804) (also PR126881, which needs `-O1` for the defect to manifest as wrong code)
**Affects:** stock g++ 16.2.0 (ICEs, with or without `-O1`); g++ trunk
(17.0.0 20260901) does not reproduce this exact ICE (see Notes); older
stock g++ 13.4.0/14.4.0/15.3.0 do not support C++26 contracts syntax at
all and are not applicable

## Bug Report

A function template with a by-value variadic parameter pack (`Ts... a`,
non-reference type) that is named inside a `pre` or `post` contract
through a fold-expression ICEs the compiler's own template substitution
when instantiated with at least one argument:

```
internal compiler error: in tsubst, at cp/pt.cc:17277
```

The root cause is in how a contract's pack expansions are substituted:
`tsubst_contract` registers the pattern's parameters as specializations of
the instantiation's by walking both `DECL_ARGUMENTS` chains in lockstep,
which does not hold across a parameter pack -- one pattern `PARM_DECL`
stands for every parameter instantiated from it, so pairing it off against
a single instantiated parameter makes the pack look one element long and
clobbers the correct entry `register_parameter_specializations` already
made one frame up. This is latent while a contract's pack expansions
ignore local specializations, and became observable once an unrelated
upstream fix (PR c++/125645) made them consult local specializations.

## Reproducer

See [`gcc-20-pack-by-value-in-contract.cpp`](gcc-20-pack-by-value-in-contract.cpp)
in this directory.

## Our Fix

`gcc/cp/pt.cc`, `tsubst_contract`: register the whole argument pack with
`extract_fnparm_pack`, exactly as `register_parameter_specializations`
does, and advance past the parameters it consumes, instead of walking both
parameter chains one element at a time.

## Notes

This reproducer was **freshly constructed**, not extracted from a test,
per this task's instructions -- the fix commit does not add a new test; it
corrects an existing test's *expectations*
(`gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.res.p1-pack-empty.C`,
confirmed via `git show 3eb1e3f1974 --stat`).

The commit's own log message gives a literal reproducing shape for a
related symptom (a pack-index expression giving a bogus "pack index '1' is
out of range for pack of length '1'" error):

```cpp
template <typename... Ts>
void f (Ts... a) post (a...[1] == 0) {}
void g () { f<int, const int> (1, 2); }
```

That literal example was tried first against stock g++ 16.2.0 and did
**not** reproduce: the pack must be `const` to pass the (unrelated and
correctly-enforced) "value parameter used in a postcondition must be
const" check first, and once made `const`, this exact shape compiled
cleanly on both stock 16.2.0 and trunk -- no ICE, no diagnostic. A
fold-expression shape (`(... && (a > 0))`, as used in this branch's own
`dcl.contract.res.p1-pack-empty.C`) was tried instead and reliably ICEs
stock 16.2.0's `tsubst`, with or without `-O1`, which is what this file
ships as the reproducer.

g++ trunk (17.0.0 20260901) does **not** reproduce this specific ICE with
this reproducer -- it compiles cleanly. This is plausible: trunk
presumably already carries the upstream fix for PR c++/125645 (the change
that exposed this class of bug) in a form that does not share this
branch's own pack-registration code, so trunk's behavior here says nothing
about whether PR126804/PR126881 are otherwise still open upstream in some
other shape; this write-up takes the task's own information (that they are
open, filable reports matching this fix commit) at face value, since
Bugzilla itself could not be independently browsed in this session.

PR126881's specific note that `-O1` is required to see wrong code rather
than "just" an ICE was not separately reproduced: this reproducer ICEs
outright at both `-O0` and `-O1` on stock 16.2.0, which is a stronger
(if less subtle) demonstration of the same underlying defect than the
wrong-code path PR126881 describes.
