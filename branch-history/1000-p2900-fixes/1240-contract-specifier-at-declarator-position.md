---
id: 1240-contract-specifier-at-declarator-position
subject: 'c++: contracts: P2900: parse the specifier-seq at its grammar position'
depends: []
regenerates: []
fixes: [gcc-45]
---

## Rationale

A `function-contract-specifier-seq` follows the complete **declarator**.  The
parser attached it to whatever `parameters-and-qualifiers` immediately
preceded it, in `cp_parser_direct_declarator`'s `CPP_OPEN_PAREN` arm.  Those
two positions coincide for an ordinary function -- which is why the feature
worked at all -- and diverge for every declarator whose outermost part is
something else, in three directions at once: accepted on a typedef or a
pointer, silently **dropped** on a trailing-return `type-id`, and **refused**
after an array declarator or wherever the predicate names a parameter.

    int (*f (int i)) (int) pre (i > 0);    // 'i' was not declared
    auto f (int i) -> int (*) (int) pre (i > 0);   // accepted, never checked

Parse it instead at the four grammar positions that carry it --
`init-declarator`, `function-definition`, `member-declarator` and
`lambda-declarator` -- attaching it to the `cdk_function` nearest the
declarator-id, which is the one `grokdeclarator` visits last and so the one
that declares the function.  A position with no declarator to attach to
diagnoses instead of dropping: `cp_parser_reject_contracts` names the
construct.

Moving the parse forces the predicate to be **deferred**.  Declarators are
built inside-out and a function's own parameter list need not be the last
one, so in `int (*f (int i)) (int) pre (i > 0)' the scope the predicate needs
was destroyed two steps before the contract was seen: there is no position in
the parse at which the right parameters are live.  The predicate is therefore
token-cached -- the mechanism in-class members already used -- and replayed
once the FUNCTION_DECL exists and the parameters can be put back.  A
lambda-declarator is the exception: its parameter list *is* the declarator,
so its scope is still open and it parses in place.

The replay runs from `start_function_contracts`, the only point that
satisfies all three constraints: after `store_parm_decls`, so the parameters
are in scope; before `handle_contracts_p`, which is false under
`processing_template_decl` and would leave a template's pattern unparsed for
`tsubst_contract` to meet at instantiation; and gated on actually parsing a
definition, so a template *instantiation* -- which reaches the same function
with contracts copied from the pattern and no token cache of its own -- does
not replay unrelated lexer state.  A declaration with no body replays from
`cp_parser_init_declarator` with `inject_parm_decls` around it.

Everything the declarator used to parse eagerly inside that scope follows:
postcondition captures are resolved and diagnosed at the late parse, and a
result-name `attribute-specifier-seq` travels on the contract node, because
the variable it applies to does not exist until then.

Redeclaration matching is rebuilt around the same fact.  It was skipped
whenever either side was still deferred -- survivable while only friend
declarations were, and not once everything is.  Matches are queued with the
two contract vectors and the redeclaration's parameter **names**, and run
once both sides are parsed.  The redeclaration itself does not survive to be
parsed: `duplicate_decls` drops its `contract_decl_map` entry and frees it,
so the queued predicate is parsed against the surviving declaration's
parameters, bound under the names the redeclaration wrote, so a renamed
parameter resolves positionally.  Only a differing parameter *count* defeats
that, which two declarations of one function cannot do.

Three further consequences of that rebuild:

* Adding contracts to a function first declared without them is diagnosed
  once the predicate has parsed, not before, and the refused contracts are
  dropped rather than recorded -- so a second redeclaration is still compared
  with the first declaration rather than with contracts that were refused.
* A by-value parameter of non-trivially-copyable class type is retyped to a
  reference by `cp_genericize`, so after a function is **defined** every
  expression built from that parameter carries an indirection.  The throwaway
  match parse borrows the parameters' types back, as it already borrowed
  their names, or two textually identical contracts compare unequal.
* A **qualified friend** declaration redeclares a member that already exists:
  `check_classfn` returns the member and the friend declaration is discarded
  without `duplicate_decls` running, which is the only place a
  redeclaration's contracts were checked.  `do_friend` checks it directly.

`grokdeclarator` keeps the "declares no function" diagnosis, now reached from
a position where the answer is unambiguous, and loses its "cannot appear on a
return type" error, which rejected the well-formed
`int (*f (int)) (int) pre (true)'.

## Compile gap

The parse positions, the deferral, the late replay and the matching queue all
land together, and they have to: a tree that moved the parse without the
replay would token-cache predicates nothing ever reads.

**Forward reference to `2400-p3595`.**  Carrying a result-name
`attribute-specifier-seq` to the late parse needs somewhere on the node to
put it, and that is `POSTCONDITION_RESULT_ATTRS` -- operand 14 of a
15-operand `POSTCONDITION_STMT`.  Both the operand count in `cp-tree.def` and
the accessor in `contracts.h` are declared by the P3595 configuration work,
which rewrote that node's whole layout, so this commit uses the accessor
several commits before it exists and does not build until `2400-p3595`
lands.  Splitting the operand out to here would mean this commit editing a
layout it does not otherwise touch, and would leave P3595's own
renumbering to collide with it.

The requires-clause on a contract assertion (P4283) needs the same deferral
and gets it in `4630-label-and-requires-parsing`, which introduces the clause
in the first place; nothing here refers to it.

## Contents

- gcc/cp/contracts.cc : /^contract_all_valid_p \(/, /True if every contract in CONTRACTS parsed/, /A contract whose predicate never parsed/, /A redeclaration match that could not be performed where redeclarations are/, #99, #100, @contract_parm_name_list, @contract_parm_count_matches_p, @flush_deferred_contract_matches, /Whether there is anything to complain about is not yet known when/, /A friend redeclaration is not special here/, /One side has not been parsed yet, so the two cannot be compared/, /The copy is unconditional on purpose/
- gcc/cp/contracts.h : /Late-parse the deferred predicates of a redeclaration's CONTRACTS as if/
- gcc/cp/decl.cc : /At most one cdk_function in the chain carries a seq/
- gcc/cp/friend.cc : *
- gcc/cp/parser.cc : /^#include "stor-layout\.h"$/, /True only while start_function is running for a function definition the/, /^  \(cp_parser \*, bool\);$/, /^static tree cp_parser_declarator_contracts_opt$/, @cp_parser_lambda_declarator_opt, @cp_parser_alias_declaration, @cp_parser_init_declarator, @cp_parser_direct_declarator, /A parameter-declaration takes no function-contract-specifier-seq/, @cp_parser_member_declaration, /Apply any attribute-specifier-seq written on the result name/, /Late-parse the deferred predicates of CONTRACTS as if they belonged to/, @cp_parser_late_parse_for_match, @cp_late_parse_function_contracts, /^cp_parser_function_contract_specifier \(cp_parser \*parser, bool defer\)$/, /DEFER is set by every caller that parses the seq at its grammar/, /^cp_parser_function_contract_specifier_seq \(cp_parser \*parser, bool defer\)$/, /Diagnose and consume a function-contract-specifier-seq written somewhere/, /cp_parser_function_contract_specifier \(parser,$/, @cp_parser_reject_contracts, /^declared_function_declarator \(cp_declarator \*declarator\)$/, @cp_parser_declarator_contracts_opt, @cp_parser_function_definition_from_specifiers_and_declarator
- gcc/testsuite/g++.dg/contracts/cpp26/contract-declarator-positions-run.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contracts-nested-class1.C : *
