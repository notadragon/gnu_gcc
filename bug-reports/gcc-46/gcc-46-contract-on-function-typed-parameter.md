# GCC-46: A contract specifier on a parameter of function type is accepted and silently dropped

**Status:** Fixed here
**Resolved by:** `1250-contract-on-non-function-declarator`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-09-16). The whole
`component:c++ contracts` list was enumerated -- 65 bugs -- and filtered for
`typedef`, `alias`, `declarator`, `non-function`, `pointer`, `parameter of`,
`silently`, `ignored`, `accepted` and `drop`; the only two hits are
PR127255 and PR127290, both of which are ours and neither about declarator
placement.  Targeted queries were run as well (`contracts typedef`,
`contracts declarator`, `function-contract-specifier`, `contracts ignored`,
`contracts non-function`, `contract typedef-name`,
`contract specifier parameter`, `keywords:accepts-invalid contract`).

The nearest existing report is
[PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486)
("[contracts] Missing restriction on defaulted and deleted functions"),
which is the same family -- a missing restriction on where a contract may be
written -- but a different restriction.

Worth reading alongside, because it is the same defect for the sibling
specifier: the requires-clause is accepted in the wrong declarator position
too, at three points, tracked upstream as
[PR123909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123909) and
[PR94984](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94984) and here as
GCC-47.
**Affects:** measured 2026-09-16 -- accepted on `16.1.0`, `16.2.0` and trunk
`17.0.0 20260914 (experimental)` (`b76fde4b175`), and on this branch.
`13.4.0`, `14.4.0` and `15.3.0` do not accept the `pre`/`post` syntax at all.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a contract specifier on a function-typed parameter is accepted and silently ignored` |

Attachments:

| File | Description |
|---|---|
| [`contract-on-function-typed-parameter.cpp`](contract-on-function-typed-parameter.cpp) | Six shapes, all accepted with no diagnostic |

````
A function-contract-specifier-seq written on a parameter whose declared type
is a function type is accepted and then silently discarded.

Such a parameter is adjusted to a pointer to function ([dcl.fct]/5), so it
declares no function at all.  A function-contract-specifier-seq is part of a
function declarator ([dcl.contract.func]), and there is none here -- no call
through the resulting pointer is, or could be, checked against the predicate.
The program should be rejected.

```
void takes_fn (int bar () pre (true));

void takes_fn_post (int bar () post (r : r > 0));

void takes_fn_args (int bar (int, int) pre (true));

void defines_fn (int bar () pre (true)) { (void) bar; }

struct S {
  void mem (int bar () pre (true));
};

template <typename T>
void tmpl (T bar () pre (true)) { (void) bar; }

template void tmpl<int> (int bar ());
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only contract-on-function-typed-parameter.cpp; echo $?
0
```

No diagnostic of any kind is produced.  Note in particular `defines_fn`: the
contract sits on a parameter of a function that IS being defined, so a reader
can very reasonably expect it to be checked, and it is not.

This is the worst failure mode a contract facility has: the user writes a
precondition, is told nothing, and gets no check.

For contrast, the same mistake on an ordinary parameter is rejected, so the
restriction exists and is merely not reached for the function-typed case:

```
void takes_int (int bar pre (true));   // error: expected ',' or '...' before 'pre'
```


DISCOVERY

Found while comparing this test suite against an independent Clang contracts
implementation, which rejects every shape above with a dedicated diagnostic.
Clang -- both stock-with-contracts work and our own branch -- rejects this
one already, though only as a generic `expected ')'` cascade rather than a
message about contracts.


ANALYSIS

The contract is parsed and attached, and then dropped because a parameter
never becomes a function -- but the interesting part is that the check which
should have caught it exists and is simply ordered too early.

cp_parser_direct_declarator (gcc/cp/parser.cc) parses a
function-contract-specifier-seq after ANY parameter list, guarded only by
flag_contracts.  A parameter declared with a function declarator is parsed by
the same function recursively, so the contract lands on the PARAMETER's own
cdk_function node.

grokdeclarator (gcc/cp/decl.cc) accumulates it as usual, then reaches

  if (!FUNC_OR_METHOD_TYPE_P (type))
    {
      ...
      if (reqs)
        error_at (location_of (reqs),
                  "requires-clause on declaration of non-function type %qT",
                  type);
    }

which is exactly the right question and does not fire: at that point `type`
is still the bare FUNCTION_TYPE.  The function-to-pointer adjustment for
parameters ([dcl.fct]/5) happens further down, in the `decl_context == PARM`
handling --

  else if (TREE_CODE (type) == FUNCTION_TYPE)
    type = build_pointer_type (type);

-- after the guard that depends on it.  grokdeclarator then builds a
PARM_DECL and the accumulated specifiers are never read.

This ordering is why `void f (int bar () requires true);` is ALSO accepted
and silently dropped, which is worth stating plainly: the hole is not
specific to contracts, and a fix that only adds a contract check beside the
existing requires-clause one would leave the requires-clause case as broken
as it found it.  Testing decl_context == PARM directly, rather than the type,
is what closes both.

set_fn_contract_specifiers (gcc/cp/contracts.cc) never asserts that its key
is a FUNCTION_DECL, so nothing downstream notices that no specifiers were
recorded for the enclosing function either.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepted
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260914, b76fde4b175  yes

13.4.0, 14.4.0 and 15.3.0 do not accept the pre/post syntax at all.
````

## Reproducer

See [`contract-on-function-typed-parameter.cpp`](contract-on-function-typed-parameter.cpp)
in this directory.  No headers, so no `.ii` is needed.

## Notes

Fixed on this branch by `1250-contract-on-non-function-declarator`, which
closes this shape and four others in one check; the row survives here
because upstream still accepts it.

Related but separately tracked: [GCC-45](../gcc-45/gcc-45-contract-on-typedef-declaration.md),
a contract in a typedef declaration.  See that entry's Notes for why the two
are kept apart rather than merged.

The watch test is
`gcc/testsuite/g++.dg/contracts/cpp26/open-bug-contract-on-function-typedef.C`,
which xfails this shape and GCC-45's together.  There is no Clang mirror of
this row specifically: Clang already rejects it.
