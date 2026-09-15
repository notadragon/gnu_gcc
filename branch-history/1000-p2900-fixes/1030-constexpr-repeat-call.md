---
id: 1030-constexpr-repeat-call
subject: 'c++: contracts: let a contract condition re-call a constexpr function'
depends: [1020-constexpr-side-effect]
regenerates: []
fixes: [gcc-03]
---

## Rationale

GCC-3 (PR c++/125459).  Membership of a `modifiable_tracker`'s "may not be
modified" set was decided by whether the object was already a key in the
value map.  But `destroy_value` retires a `VAR_DECL`/`PARM_DECL`/`RESULT_DECL`
by leaving it in that map mapped to `void_node` -- or to `void_list_node`
when it is past the end of its storage -- rather than by removing it.  A
`constexpr` function called once, returned from, and then called *again* from
inside a contract condition therefore found its own `RESULT_DECL` already
present, was refused permission to initialise it, and the condition came out

    error: contract condition is not constant

under `enforce`, and the same text as a warning under `observe`.  Found by the
BDE contracts integration, where a formatter's parse loop tests
`spec.empty ()` and then calls a helper asserting `!spec.empty ()`.

`put_value` now asks whether the object is *currently alive* rather than
whether it has ever been seen: absent, `void_node` or `void_list_node` all
mean the store begins a new object, which as far as the tracker is concerned
belongs to the subexpression being tracked exactly as a never-before-seen
object does.  Only a live object was genuinely created outside.

A reviewer should check both dead markers are tested -- every other reader in
the class tests for both, and testing only `void_node` reintroduces the bug
for an object past the end of its storage -- and that the extra map lookup is
performed only when a tracker is active, which is rare, so the common path
still costs one hash operation.

## Compile gap

None.  This is a self-contained change to one `put_value` body.

It is placed after `1020-constexpr-side-effect` only because the two edit
adjacent parts of the same tracker and reviewing them in the other order means
reading this one against a `modifiable_tracker` that still rejects the
program outright; nothing here needs 1020's fields.

## Contents

- gcc/cp/constexpr.cc : /An object whose lifetime has ended stays in the map/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-repeat-call.C : *
