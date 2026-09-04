# GCC-17: `this` accepted in the trailing return type of an explicit-object member function's own declaration

**Status:** Open
**Component:** c++ / declarations
**Upstream Link:** --
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

## Our Fix

None -- genuinely upstream's. Found while fixing the contracts-specific
analog; that fix was deliberately kept at the contracts call site so this
shared, non-contracts behavior stays reproducible and untouched.

## Notes

Shares its root cause with CLANG-8 in the `llvm_llvm-project` fork; file
both together and cross-reference each other when reporting upstream.
