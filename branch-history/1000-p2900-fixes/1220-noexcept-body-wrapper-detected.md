---
id: 1220-noexcept-body-wrapper-detected
subject: "c++: contracts: don't assume a noexcept body is wrapped in MUST_NOT_THROW"
depends: []
regenerates: []
fixes: [gcc-24]
---

## Rationale

GCC-24 (PR c++/127173).  A `noexcept` function carrying a contract segfaulted
the compiler under `-fno-enforce-eh-specs`:

    void f (int x) noexcept pre (x > 1) { }

`maybe_apply_function_contracts` has to find the user's written body so it can
splice the checks around it, and it predicted the body's *shape* from the
function's exception specification: a `noexcept` function's body "will be
wrapped in a MUST_NOT_THROW expression", so it took `expr_first
(DECL_SAVED_TREE (fndecl))` and used that wrapper's first operand.  The
prediction is not sound.  The wrapper comes from `begin_eh_spec_block`, which
`use_eh_spec_block` gates on `flag_enforce_eh_specs` among other things, so
under `-fno-enforce-eh-specs` there is no wrapper at all and whatever tree
happened to come first in the body was read as if it were one.  A checking
build trips the `gcc_checking_assert`; a release build -- which is what both
stock 16.2.0 and trunk are, and what the reporter had -- follows
`TREE_OPERAND` off a tree of the wrong code and segfaults.

The fix tests the body for the wrapper rather than inferring it: compute
`m_n_t_expr` only when the function is `noexcept`, and take the wrapper branch
only when what came back is really a `MUST_NOT_THROW_EXPR` (and not
`error_mark_node`).  What a reviewer should check is that this is a strict
widening of the old condition and nothing else: the wrapped case behaves
exactly as before, and the previously-undefined unwrapped case now falls into
the same arm a non-`noexcept` function takes, which is the arm that was always
correct for a body with no wrapper.  That also silently covers the other
reasons `use_eh_spec_block` declines to wrap a body -- a cloned function, or an
implicitly-generated constructor or destructor -- which were latent behind the
same faulty assumption and needed no separate change.

## Compile gap

None.  Every name involved -- `expr_first`, `TYPE_NOEXCEPT_P`,
`MUST_NOT_THROW_EXPR`, `error_mark_node` -- is upstream, and the three hunks
are a self-contained rewrite of one `if` head inside a function that already
exists at the base.

## Contents

- gcc/cp/contracts.cc : #71, #72, #73
- gcc/testsuite/g++.dg/contracts/cpp26/pr127173.C : *
