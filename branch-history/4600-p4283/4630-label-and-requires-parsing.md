---
id: 4630-label-and-requires-parsing
subject: 'c++: contracts: parse the label and the requires-clause of a contract assertion'
depends:
  - 4200-p3400-core
  - 4600-p4283
regenerates: []
fixes: []
---

## Rationale

An assertion-control label (P3400) and a requires-clause (P4283) occupy the
same slot in the grammar -- between the contract introducer and the predicate,
in that order -- so the three places that walk that slot walk both at once and
cannot be split by paper.

* `cp_parser_contract_assert' and `cp_parser_function_contract_specifier' each
  gain the same two lines: call `cp_parser_assertion_control_specifier', then
  `cp_parser_contract_requires_clause', then recover from an ill-formed
  requires-clause by skipping the predicate's paren group.  The two calls are
  one hunk apiece.

* `cp_maybe_function_contract_specifier' -- the lookahead that decides whether
  a `pre'/`post' is a contract at all, before any tentative parse -- has to
  step over both without parsing them.  Neither is trivially delimited: a
  label's constant-expression may contain `<', `>' or `;' inside a
  parenthesized subexpression or a lambda body, and a requires-clause need not
  be parenthesized at all, so `pre requires Foo<T> (x > 0)' has to be
  distinguished from the predicate that follows it.  `cp_skip_balanced_group'
  is the shared helper both scans use to step over a bracketed group
  atomically; it exists only for them.

Placed after P4283, the later of the two papers.

## Compile gap

None: `cp_parser_assertion_control_specifier' comes with `4200-p3400-core' and
`cp_parser_contract_requires_clause' with `4600-p4283'.  Before this commit
both are defined and unreferenced, so neither spelling is accepted yet.

## Contents

- gcc/cp/parser.cc : @cp_function_contract_specifier_intro, /Parse optional assertion-control-specifier/, /Skip optional assertion-control-specifier/
