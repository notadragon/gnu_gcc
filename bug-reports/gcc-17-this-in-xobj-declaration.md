# GCC-17: `this` accepted in the trailing return type of an explicit-object member function's own declaration

**Status:** Open
**Component:** c++ / declarations
**Upstream Link:** None found. Searched 2026-09-05, including resolved bugs:
summary `explicit object` (25 hits, none about `this` being accepted where it
must be rejected), summary `trailing return` (20 hits, the closest being
PR53721 which is the opposite direction -- `this` wrongly *rejected* in an
ordinary member's trailing return type), and comment text
`explicit object trailing return this`. PR125330 looked promising and is not
this: it is the explicit object *parameter* being unavailable in a trailing
requires-clause, already fixed
**Affects:** stock g++ 14.4.0, 15.3.0, 16.2.0, trunk (13.4.0 predates
explicit object parameters and rejects the reproducer outright)

## Bug Report

[expr.prim.this]/3 forbids `this` "within the declaration" of an
explicit-object member function, but GCC accepts it in the trailing return
type (e.g. `auto f (this S &self) -> decltype (this->x)`), while correctly
rejecting the identical shape on a `static` member function (the control).
This is not contracts-related; it reproduces on stock g++ from 14.4.0
through trunk (13.4.0 predates explicit object parameters and rejects the
reproducer outright). It shares its root cause with CLANG-8 in the
`llvm_llvm-project` fork -- both reports should be filed together,
cross-referencing each other.

## Reproducer

See [`gcc-17-this-in-xobj-declaration.cpp`](gcc-17-this-in-xobj-declaration.cpp)
in this directory.

## Why it matters for contracts

The trailing return type is where a postcondition's **result binding** gets
its type, so this accepted-invalid declaration feeds one:

```cpp
struct D : S {
  auto f (this D &self) -> decltype (this->x) post (r : r == 0) { return self.x; }
};
```

Accepted by stock g++ trunk, by this branch, and by our Clang: `r` takes its
type from a declaration that should have been rejected outright. Nothing
downstream of the bad declaration misbehaves on its own -- this is GCC-17
showing through contracts, not a second defect -- but it is the concrete cost
of leaving it unfixed, and worth stating when the report is filed.

## Our Fix

None -- genuinely upstream's. Found while fixing the contracts-specific
analog, which is [GCC-28](gcc-28-xobj-member-in-predicate-ctor-message.md):
an unqualified member named in an explicit-object member function's contract
predicate was diagnosed with a constructor/destructor message. The two are
the non-contracts and contracts-specific halves of the same sentence in
[expr.prim.this]/3. That fix was deliberately kept at the contracts call site
so this shared, non-contracts behavior stays reproducible and untouched.

## Notes

Shares its root cause with CLANG-8 in the `llvm_llvm-project` fork; file
both together and cross-reference each other when reporting upstream.
