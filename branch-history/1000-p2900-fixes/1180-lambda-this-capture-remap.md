---
id: 1180-lambda-this-capture-remap
subject: "c++: contracts: P2900: do not remap a lambda's captured-this proxy"
depends: []
regenerates: []
fixes: [gcc-10]
---

## Rationale

GCC-10 (PR c++/127283).  A contract parsed on a *declaration* has no
`FUNCTION_DECL` yet, so its `this` is a dummy that `remap_dummy_this_1`
rewrites to the real `this` once the definition is known.  The walk selected
its targets with `is_this_parameter`, which is also true of a lambda's
captured-`this` proxy -- a `VAR_DECL` named `this` whose `DECL_VALUE_EXPR` is
already `__closure->__this`.  Rewriting that to `DECL_ARGUMENTS`, which in a
lambda's `operator()` is `__closure` and not an object of the enclosing class,
made a predicate on such a lambda read the closure object as if it were the
enclosing class.

The walk now requires a `PARM_DECL`.  The proxy needs no remapping; only the
dummy `this` of a contract parsed on a declaration does.

## Compile gap

None: one added type test in an existing walker.

## Contents

- gcc/cp/contracts.cc : @remap_dummy_this_1
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-this-predicate.C : *
