---
id: 1060-postcondition-odr-use
subject: 'c++: contracts: P2900: decide postcondition odr-use on the finished predicate'
depends: [1050-predicate-lambda-constify]
regenerates: []
fixes: [gcc-19, gcc-36, gcc-650, gcc-663]
---

## Rationale

[dcl.contract.func]/7 requires a by-value parameter that a postcondition
predicate *odr-uses* to be `const`.  Upstream decided that per id-expression,
from `finish_id_expression_1` and `tsubst_expr`, at the moment the parameter
was named.  Whether an id-expression is an odr-use is a property of the
expression built around it, which does not exist yet at that point, so the
per-name test was structurally unable to be right.

GCC-19 (PR c++/126897) is the visible consequence: in `p, q` the discarded
operand `p` is not an odr-use of `p` ([basic.def.odr]/2 exempts the potential
results of a discarded-value expression), yet the parameter was diagnosed.
GCC-36 (PR c++/127297) is the other side of the same defect -- for a
non-template the per-name test fired twice, emitting the identical diagnostic
at the identical location, and this rework makes it single-shot as a
by-product rather than by design.

The rule now runs once, over the finished predicate.
`check_postcondition_param_odr_uses` walks the condition with
`check_postcondition_odr_use_r`, carrying the function and location in a
`postcondition_odr_use_data`; `walk_discarded_operand` implements the
potential-results exemption, walking everything in a discarded operand that is
*not* a potential result -- an argument to a call, the condition of a `?:` --
because those are evaluated and do odr-use what they name.  The two entry
points are the places every predicate finally passes through:
`update_late_contract` for a late-parsed contract, and `tsubst_contract` for
an instantiated one, where a pack-index-expression has already selected its
element.

A reviewer should check the exemption list in `walk_discarded_operand`
against [basic.def.odr]/2 directly, and check that `update_late_contract` is
the right single hook -- it is, because a postcondition with no
result-name-introducer never reaches the result-variable rebuild, which was
the other candidate.

A member access naming a non-static data member contributes its object
expression's potential results, so a by-value parameter named only as the
object of a discarded member access (`post ((s.m, true))`) is not odr-used
either, unless the access is volatile (GCC-650,
`postcondition-discarded-member-access.C`).

In a template, a member access through a type-dependent object
(`post ((s.m, true))` with `s` of type `const T`) is built with the member
still a name, so whether it names a non-static data member is not known:
the object is taken as a potential result there, and the walk over the
substituted predicate decides (GCC-663, `postcondition-discarded-member-template.C`).

The object of an access to a static member or an enumerator is not among the
access's potential results, so a parameter that is that object is odr-used;
`build_class_member_access_expr` and `keep_unused_object_arg` drop an object
without side effects before the walk could see it, so
`check_postcondition_object_odr_use` applies the rule to it where it is
dropped (GCC-666).

## Compile gap

None of its own.  `update_late_contract` gains an `fndecl` parameter here and
its one caller in `cp_parser_late_contract_condition` is updated in the same
commit.

One hunk in `rebuild_postconditions` carries this commit's comment about /7
having already been applied, and, on the same line, the rename of
`processing_postcondition` to `processing_postcondition_predicate`.  That
variable is renamed by `4000-p3098`; standing alone this commit would have to
keep the old spelling, which 4000 then overwrites.  The three neighbouring
hunks performing the same rename stay in `1000-p2900-fixes`.

## Contents

- gcc/cp/contracts.cc : @postcondition_odr_use_data, @check_postcondition_odr_use_r, @walk_discarded_operand, @check_postcondition_object_odr_use, #29, #30, @check_param_in_postcondition:0, #151, #167, #169
- gcc/cp/contracts.h : @make_postcondition_variable:0, /extern void check_postcondition_object_odr_use/
- gcc/cp/typeck.cc : /check_postcondition_object_odr_use \(object\);/
- gcc/cp/call.cc : /check_postcondition_object_odr_use \(obj\);/
- gcc/cp/parser.cc : /update_late_contract \(contract, fn, result, parsed_condition, comment\);/
- gcc/cp/pt.cc : /check_postcondition_param_odr_uses \(CONTRACT_CONDITION \(r\), decl, cond_l\);/
- gcc/testsuite/g++.dg/contracts/cpp26/pr126897.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-discarded-operand-dependent.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-unevaluated-operand.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-const-function-pointer-param.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-function-param-reason.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-discarded-member-access.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-discarded-member-template.C : *
