---
id: 2020-defarg-cache-mode
subject: 'c++: parser: give cp_parser_cache_defarg a mode instead of a bool'
depends: []
regenerates: []
fixes: []
---

## Rationale

`cp_parser_cache_defarg` collects a deferred token range and has to solve the
Core-issue-325 ambiguity while doing it: a `<' inside the range might open a
template-argument-list, whose commas must not be mistaken for the delimiter
that ends the range.  Upstream it serves two callers, distinguished by a
`bool nsdmi'.

P3098 postcondition init-captures need a third: `[p = f<1, 2>()]' has exactly
the same ambiguity, resolved differently again (the range ends at a comma only
when another capture follows).  A third caller cannot be spelled as a second
boolean, so this commit turns the parameter into an enumeration --
`cp_defarg_cache_defarg', `cp_defarg_cache_nsdmi', `cp_defarg_cache_capture' --
and derives the old `nsdmi' local from it.

Behaviour-neutral: the two existing callers pass the enumerators that stand for
what their boolean said, and the function's only use of the parameter is the
same `nsdmi' test as before.  The capture enumerator is declared here and used
by `4000-p3098'; separating the mechanical change from the behaviour lets a
reviewer of the postcondition-capture work read only the new arm.

## Compile gap

None.  `cp_defarg_cache_capture' has no caller and no arm of its own until
`4000-p3098'; it is an unused enumerator, which is not diagnosed.

## Contents

- gcc/cp/parser.cc : /cp_defarg_cache_defarg/, /cp_defarg_cache_mode/, /cp_defarg_cache_nsdmi/
