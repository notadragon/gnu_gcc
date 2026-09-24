---
id: 1250-contract-on-non-function-declarator
subject: "c++: P2900: reject a contract or requires-clause on a non-function declarator"
depends:
  - 1240-contract-specifier-at-declarator-position
regenerates: []
fixes: [gcc-45, gcc-47]
---

## Rationale

GCC-45 and GCC-47: two specifiers, one rule, and one hole in it.

A function-contract-specifier-seq may not be written unless the declarator
declares a function ([dcl.decl.general]/6), and a requires-clause may only
trail a declarator that declares a templated function ([dcl.decl.general]/5).
Both travel in the same declarator slot, and both were accepted on
declarators that declare no function and then silently discarded -- no
diagnostic, and no check or constraint at any use.  That is the one failure mode neither
facility may have: the user writes a precondition or a constraint, is told
nothing, and gets nothing.

Five declarators take a contract specifier anyway, and they are one bug.
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

The requires-clause is refused at three of those points already --
"requires-clause on typedef", "on type-id", "on declaration of non-function
type".  Its parameter case is not, and copying those checks verbatim would
have reproduced the hole: the guard they hang on,
`!FUNC_OR_METHOD_TYPE_P (type)`, is ordered BEFORE the function-to-pointer
adjustment that would make it true ([dcl.fct]/5), so a parameter still looks
like a `FUNCTION_TYPE` there.  Asking about `decl_context` rather than about
the type is what closes it, and it closes it for both specifiers at once.

**The classification is asked once.**  `classify_non_function_declarator`
answers "what does this declarator declare, if not a function?" from
`typedef_p`, `decl_context` and the type -- all three final at that point --
and both specifiers read its answer.  Having one answer is what keeps the
two from drifting apart as either rule is extended.

They remain inconsistent in their WORDING, and deliberately so: the
requires-clause's three existing messages are upstream's, and this commit
does not re-spell them.  The contract diagnostic names the declarator kind
rather than saying "not a function", because the five shapes fail for
visibly different reasons and a reader needs to know which one they hit.

`FIELD` is deliberately not rejected wholesale: a member function
declaration arrives with `decl_context == FIELD`, the same context as the
data member two rows above it in the test, so the discriminator has to be
the type and the storage class rather than the context alone.

## Compile gap

None.  The classification helper, its enumeration and both specifiers'
checks land together; the two tests need `-fcontracts` and C++20
respectively, and neither needs the other.

`classify_non_function_declarator` is static rather than living in
`contracts.cc` with `check_contract_on_defaulted_or_deleted`, which is
otherwise the model for this kind of rule, for a mechanical reason: it needs
`enum decl_context`, which is declared in `decl.h`, and `contracts.h` does
not include it -- an unscoped enumeration cannot be forward-declared, so
exporting the helper would mean widening that header for one caller.

## Contents

- gcc/cp/decl.cc : /What kind of thing a declarator declares/, @non_function_declarator, /^non_function_declarator_name \(/, /^contract_on_non_function_p \(/, /Refuse a contract on a declarator that will not become a function/
- gcc/testsuite/g++.dg/concepts/requires-clause-on-parameter.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-on-non-function-declarator.C : *
