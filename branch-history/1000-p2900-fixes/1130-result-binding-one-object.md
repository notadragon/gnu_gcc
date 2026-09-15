---
id: 1130-result-binding-one-object
subject: "c++: contracts: give a postcondition's result binding one addressable object"
depends: []
regenerates: []
fixes: [gcc-12, gcc-33]
---

## Rationale

Two bugs, one object.  [dcl.contract.res]/1 binds a postcondition's result
name to *the* returned object; GCC gave it none, or gave it a different one
each time the predicate took its address.

GCC-12 (PR c++/112794).  A scalar result is a gimple register, so
`gimplify_addr_expr` spills it to a fresh temporary every time a predicate
takes its address -- and a predicate may do so more than once:

    int f () post (r : rec (&r) && rec2 (addr_via_ref (r)))

gives two spills and therefore two addresses for one result binding, inside a
single evaluation of a single predicate.  Nothing permits two objects, and the
predicate's value comes out wrong.

GCC-33 (PR c++/125574) is the same gap seen from codegen: a class-type result
binding passed to a function taking a reference needs an address the binding
does not have, and the compiler ICEs in `expand_expr_addr_expr_1`.
`g++.dg/coroutines/pr110872.C` was carrying a `dg-ice` for it; the coroutine
in that test is incidental, three lines with an ordinary function reproduce
the same ICE, so the test becomes the run test its own note asked to become.

`postcondition_needs_retval_temp_p` decides when a stand-in is required, and
`apply_postconditions` declares `__contract_retval`, initialises it from
`DECL_RESULT`, lets the checks run against it, and copies it back afterwards.
Three conditions in that predicate are worth a reviewer's attention.  A result
returned in memory is excluded -- `DECL_RESULT` is addressable and is the
object the caller sees, so copying it would be wrong.  A non-trivially-copyable
type is excluded because the copy is a bare `INIT_EXPR`; under the Itanium ABI
anything less is returned in memory and has already been excluded.  And the
copy-back is not optional: a stand-in copied into but never copied back moves
a direct `const_cast<int&>(r)++` off `DECL_RESULT` and loses the mutation that
`expr.prim.id.unqual.p7-4.C` pins -- which is worse than giving the result no
home at all.

## Compile gap

The rewritten `apply_postconditions` body is claimed here whole, and it
contains one block that is not this commit's: the loop that gates each
postcondition on its capture-initialised flag, which belongs to
`4000-p3098`.  It cannot be separated -- the retval stand-in prologue, that
loop and the copy-back epilogue are interleaved, and the function's closing
brace sits at the end of the epilogue, so any interior cut leaves a piece with
an unbalanced brace.  Standing alone, this commit would have to stub
`POSTCONDITION_CAPTURES` and `postcondition_capture_flags`, both of which
4000 then supplies for real.

## Contents

- gcc/cp/contracts.cc : @postcondition_needs_retval_temp_p, @apply_postconditions:0
- gcc/testsuite/g++.dg/contracts/cpp26/contract-result-binding-identity.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-result-binding-mutation.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-postcondition-param-rules.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr125574.C : *
- gcc/testsuite/g++.dg/coroutines/pr110872.C : *
