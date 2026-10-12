---
id: 6330-p3100-check-assume-attribute
subject: 'c++: contracts: P3100: [[assume]] as an implicit contract assertion'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:dcl.attr.assume.false`: the assumed
condition of `[[assume (E)]]` being false, which [dcl.attr.assume] makes
undefined.

No wire row and no sanitizer -- there is no runtime check to route -- but
this is the most configurable of the implicit checks, and the only one whose
*available* semantics depend on the expression rather than on the
configuration.  `build_assume_call` inspects the operand: a side-effect-free,
non-trapping predicate can be evaluated, so it reports the qualified group
`ub:dcl.attr.assume.false.pure` and the full checking set is available;
anything else reports `ub:dcl.attr.assume.false.nonpure`, only
`assume`/`ignore` apply, and a checking semantic clamps to `ignore`.  The
bare `ub:dcl.attr.assume.false` is the never-emitted parent of the two, so a
configuration can name either the whole attribute or only its checkable
half.

The three reactions are genuinely different:  `assume` is the status-quo
`IFN_ASSUME`; `ignore` drops the assumption entirely, hint included; a
checking semantic emits `if (!E) <reaction>` and, for the enforcing family
only, keeps the `IFN_ASSUME` hint as well -- the predicate provably holds
after an enforced check, but not after an observed one.  The status-quo
lowering is renamed `build_assume_ifn` and `build_assume_call` becomes the
decision in front of it.

The decision is deferred while `processing_template_decl`, because the
operand may be dependent; `build_assume_call` is called again at
instantiation with the substituted operand, and purity is judged then.

## Compile gap

None: `cp_build_assume_check` is here with its declaration, and
`resolve_implicit_contract_semantic`, the allowed-semantic clamping and the
group registry are in `6000-p3100-core`.  The
`-fcontracts-allow-assume` option, which gates whether the `assume`
evaluation semantic may be *named* at all, is also in the core, along with
its two tests -- it is a property of the semantic registry rather than of
this check.

Two traps.  First, purity is judged with `TREE_SIDE_EFFECTS` and
`generic_expr_could_trap_p`, which are conservative: an expression they
reject silently downgrades from the full checking set to `ignore`, so a
configuration asking for `enforce` gets no check and no diagnostic.  That is
deliberate -- evaluating a trapping predicate to check an assumption would
introduce the UB it is guarding against -- but it means the emitted group id
is the only way to see which half a given site landed in.  Second, keeping
the `IFN_ASSUME` hint after an enforcing check is what lets the optimizer
use the predicate; dropping it for the observing family is not an oversight
but the difference between the two families, and neither is visible in
codegen without reading the emitted IR.

## Contents

- gcc/cp/contracts.cc : @cp_build_assume_check
- gcc/cp/contracts.h : @cp_build_assume_check
- gcc/cp/cp-gimplify.cc : @process_stmt_hotness_attribute, @build_assume_call
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-assume-* : *
