---
id: 1250-contract-on-non-function-declarator
subject: "c++: contracts: reject a contract on a declarator that is not a function"
depends: [0160-requires-clause-on-parameter]
regenerates: []
fixes: [gcc-45, gcc-46]
---

## Rationale

GCC-45 and GCC-46, and three more shapes of the same defect.  A
function-contract-specifier-seq is part of a function declarator
([dcl.contract.func]/1), and writing one anywhere else was accepted and then
silently discarded -- no diagnostic, and no check at any call.  That is the
one failure mode a contract facility must not have: the user writes a
precondition, is told nothing, and gets nothing.

Five declarators were affected, and they are one bug.
`cp_parser_direct_declarator` parses a contract after ANY parameter list,
guarded only by `flag_contracts`, and stores it on the `cdk_function` node.
`grokdeclarator` accumulates it into a local and then reads that local only
on the two paths that reach `grokfndecl`; every other exit drops it.  So a
typedef, an alias-declaration, a parameter of function type, a
pointer-to-function object or data member, and an inner non-outermost
function declarator each lost it.

The parser cannot apply the rule.  At the point it parses a contract it has
not seen the decl-specifiers, so `typedef` is invisible; it does not know
whether it is inside a parameter-declaration-clause; and it builds
declarators inside-out, so an enclosing pointer declarator has not been
reached.  `grokdeclarator` knows all three, which is why the check goes
there, directly after the declarator walk completes and upstream of every
exit that would otherwise drop the specifiers.

**The requires-clause is the template, and it also shows the trap.**  A
requires-clause travels in the same declarator slot and is already refused at
these points -- "requires-clause on typedef", "on type-id", "on declaration
of non-function type".  But its parameter case is not caught, because the
guard it hangs on, `!FUNC_OR_METHOD_TYPE_P (type)`, is ordered BEFORE the
function-to-pointer adjustment that would make it true: a parameter still
looks like a `FUNCTION_TYPE` there.  This commit therefore tests
`decl_context == PARM` explicitly rather than reusing that guard.  Copying
the requires-clause checks verbatim would have reproduced the hole.

The diagnostic names the declarator kind rather than saying "not a function",
because the five shapes fail for visibly different reasons and a reader needs
to know which one they hit.

`FIELD` is deliberately not rejected wholesale: a member function declaration
arrives with `decl_context == FIELD`, the same context as the data member two
rows above it in the test, so the discriminator has to be the type and the
storage class rather than the context alone.

**The classification is not written here.**
`0160-requires-clause-on-parameter` introduces
`classify_non_function_declarator`, which answers "what does this declarator
declare, if not a function?" once for both specifiers; this commit is its
second caller and adds only the wording.  That commit goes first because a
requires-clause is C++20 and its fix stands alone, while this one needs
`-fcontracts`; asking the question once is what keeps the two specifiers'
answers from drifting apart.

They remain inconsistent in their WORDING, and deliberately so: the
requires-clause's existing messages are upstream's and neither commit
re-spells them.

## Compile gap

Needs `classify_non_function_declarator` and the `non_function_declarator`
enumeration from `0160-requires-clause-on-parameter`, which precedes it.
Nothing outward.

It is static rather than living in `contracts.cc` with
`check_contract_on_defaulted_or_deleted`, which is otherwise the model for
this kind of rule, for a mechanical reason: it needs `enum decl_context`,
which is declared in `decl.h`, and `contracts.h` does not include it -- an
unscoped enumeration cannot be forward-declared, so exporting the helper
would mean widening that header for one caller.

## Contents

- gcc/cp/decl.cc : /^non_function_declarator_name \(/, /^contract_on_non_function_p \(/, #6, #7:1
- gcc/testsuite/g++.dg/contracts/cpp26/contract-on-non-function-declarator.C : *
