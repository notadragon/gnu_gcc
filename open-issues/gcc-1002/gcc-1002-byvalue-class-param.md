# GCC-1002: a by-value class-type parameter's precondition is never proven

**Kind:** defect
**Status:** Open, this branch only -- recorded 2026-11-09
**Affects:** the P4405 proof shadow's caller-side preconditions
(`shadow_substitute`, `gcc/c-family/contracts-prove-shadow.cc`)
**Provenance:** ours -- `-fcontracts-p4405` and the proof shadow exist only
on this branch (`gcc/c-family/contracts-prove-shadow.cc` is not on
`master`)

## Symptom

```c++
struct T { int c; };
int byval (T t) pre (t.c == 1);
int byref (const T &t) pre (t.c == 1);
int scalar (int c) pre (c == 1);

int f1 () { T a = {1}; return byval (a); }   // cannot be proven
int f2 () { return byval (T{1}); }           // cannot be proven
int f3 () { T a = {1}; return byref (a); }   // proven
int f4 () { return scalar (1); }             // proven
```

`-std=c++26 -fcontracts -fcontracts-p4405 -O2
-fcontract-evaluation-semantic=prove` (or `-fcontracts-p3400` and
`pre<always_prove>`, as in the original reproducer): both calls of
`byval` are reported "contract assertion cannot be proven"; the same
precondition is proven on the const reference parameter and on the scalar
parameter.  Measured 2026-11-09 with the installed compiler at
`4847a8387e9`; `diagnose` gives the same as warnings.  Watch test
[`open-bug-p4405-byvalue-class-param.C`](../../gcc/testsuite/g++.dg/contracts/cpp26/open-bug-p4405-byvalue-class-param.C).

## Expected

Both calls proven: the argument is a known constant (or a local just
initialized with one), and the shadow sees it as plainly as for `byref`.

## Cause

A caller-side precondition reaches the shadow with the call's arguments
substituted for the callee's parameters (`shadow_substitute`), and only
an argument that `shadow_repeatable_r` finds can be evaluated again is
substituted.  A by-value class argument is a `TARGET_EXPR` --
`byval (TARGET_EXPR <D.2712, a>)`, `byval (TARGET_EXPR <D.2715, {.c=1}>)`
in the original dump -- which it refuses, so `t` stays unmapped, the
substitution fails, and `shadow_prove` returns `CPR_UNPROVEN` before
anything is deferred: the obligation is diagnosed at once, never reaching
the shadow (nor the prove-assume copy, `shadow_copy`, which takes the same
path).  A reference argument (`&a`) and a constant are repeatable.

## Rough fix scope

`gcc/c-family/contracts-prove-shadow.cc`: let `shadow_substitute` map a
by-value class parameter whose argument is a `TARGET_EXPR` of a trivially
copyable type to something the region can evaluate ahead of the call --
the `TARGET_EXPR`'s initializer when it is itself repeatable (a variable,
or a constructor of constants, which `shadow_substitute_r` and
`shadow_repeatable_r` would then have to accept), or the slot once the
argument has been evaluated, which would mean placing the marker after
the argument's evaluation rather than ahead of the call
(`diagnose_caller_side_proving` / `maybe_contract_wrap_call` in
`gcc/cp/contracts.cc`).  A non-trivially-copyable class (passed by
invisible reference, copy constructor in the argument) needs the same
thought.  `contract_prove_shadow_substitutable_p` follows
`shadow_substitute`.
