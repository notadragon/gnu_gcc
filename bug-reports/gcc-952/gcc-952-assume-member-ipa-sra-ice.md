# GCC-952: an assumption reading through a pointer ipa-sra splits ICEs in modify_call

**Status:** Open (upstream and here); deferred here by the user
2026-10-29, waiting for upstream's fix
**Component:** ipa (ipa-sra / ipa-param-manipulation); not a contracts bug
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-10-29: summary `modify_call`
and full text `ipa-param-manipulation.cc:762`, with the control full-text
`ipa-param-manipulation.cc:700` returning PR108110; related fallout of the
same series is PR128226 and PR128227, different ICEs)
**Affects:** measured 2026-10-29 -- stock trunk `17.0.0 20261029`
(`6aae4093ee77`) ICEs at `-O2`; `-fno-ipa-sra` compiles it; 16.2.0
compiles it.  This branch, rebased onto `6aae4093ee77`, ICEs the same way,
and so does its P3100 assume semantic on a member function's precondition
(`p3100-semantic-assume-run.C`).  Re-measured 2026-11-10 (rebase): stock
trunk `17.0.0 20261110` (`33ed38d4ee09`) and this branch rebased onto it
still ICE at the same assertion; Bugzilla still has no report (full-text
`ipa-param-manipulation.cc:762` empty, control `:700` returning PR108110;
PR128226 and PR128227 still open, ASSIGNED).
**Verify case:** `GCC-952` (compile, `-std=c++23 -O2`, no contracts).
**Watch test:** `g++.dg/contracts/cpp26/open-bug-assume-ipa-sra-split-ice.C`.

## Reproducer

[`gcc-952-assume-member-ipa-sra-ice.cpp`](gcc-952-assume-member-ipa-sra-ice.cpp):

```cpp
[[gnu::noinline]] bool check (int i) { return i > 0; }
struct S
{
  int v = 1;
  [[gnu::noinline]] int m (int i) { [[assume (check (i + v))]]; return i + v; }
};
int main () { S s; return s.m (2) != 3; }
```

```
during IPA pass: inline
In function 'int S::m(int)':
internal compiler error: in modify_call, at ipa-param-manipulation.cc:762
```

## Boundary (stock trunk, 2026-10-29)

| shape | result |
|---|---|
| member `m`, assume reads `i + v`, body reads `i + v` | **ICE** |
| member `m`, assume reads `i + v`, body reads `i` | **ICE** |
| member `m`, assume reads only `i` | fine |
| member `m`, assume reads only `v` | fine |
| `static int m (S *p, int i)`, assume and body read `i + p->v` | **ICE** |
| the same, assume reads only `p->v` | fine |
| the same, not `static` (externally visible) | fine |
| two `int` parameters, no pointer | fine |

So it needs a function local enough for ipa-sra to change its signature
(an inline member or a `static` function), whose outlined assumption body
takes a pointer parameter (`this`, or `p`) that ipa-sra splits into the
pointee it loads, **beside** another parameter.

## Why

The 2026-10-27 series (`c94fe046c0b` .. `684c9811dee`, "ipa: Create a
callback edge when encountering an internal .ASSUME call" and its
predecessors) gives each `.ASSUME (fn, args...)` call a callback edge to
the assumption function and lets ipa-sra change that function's signature.
`ipa_param_adjustments::modify_call` asserts that every copied parameter
still has an argument in the call's pre-existing modification map
(`mod_info->index_map[apm->base_index] >= 0`); for the `.ASSUME` call in
`m` it does not, after the caller's own signature change split the pointer.
The comment above the assertion's block says callback edges to assumption
bodies "always have a complete argument mapping"; this case is a
counterexample.

## On this branch

Nothing of ours is involved: the plain `[[assume]]` program above has no
contract.  It reaches our code through P3100's assume semantic, which
outlines a predicate the same way (the gimplifier's `[[assume]]` path), so
`p3100-semantic-assume-run.C`'s member function `S::m`, whose
precondition reads `i + v`, ICEs at `-O2`.  That test now passes
`-fno-ipa-sra` and names this row; drop the option when this is fixed.
