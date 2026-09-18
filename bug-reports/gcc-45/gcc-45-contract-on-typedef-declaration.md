# GCC-45: A contract specifier in a typedef declaration is accepted and silently dropped

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
| Summary | `[c++26][contracts] a contract specifier in a typedef declaration is accepted and silently ignored` |

Attachments:

| File | Description |
|---|---|
| [`contract-on-typedef-declaration.cpp`](contract-on-typedef-declaration.cpp) | Four shapes, all accepted with no diagnostic |

````
A function-contract-specifier-seq written inside a typedef declaration is
accepted and then silently discarded.

A function-contract-specifier-seq is part of a function declarator
([dcl.contract.func]).  A typedef declaration declares a typedef-name, not a
function, so there is no function for the contract to belong to and no call
is ever checked against it.  The program should be rejected.

```
typedef int FTypedef (int) pre (true);

typedef int FTypedefPost (int) post (r : r > 0);

struct S {
  typedef int FMember (int) pre (true);
};

template <typename T>
struct U {
  typedef T FDependent (T) pre (true);
};

template struct U<int>;
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only contract-on-typedef-declaration.cpp; echo $?
0
```

No diagnostic of any kind is produced, and the contract has no effect on any
function later declared through the typedef-name.

This is the worst failure mode a contract facility has: the user writes a
precondition, is told nothing, and gets no check.  A rejected program is
fixed in seconds; a silently dropped precondition is indistinguishable from a
working one until it matters.

The restriction IS implemented, and simply is not reached here -- USING the
typedef-name as a declarator and writing the contract there is diagnosed:

```
typedef int FTypedef (int);
FTypedef g pre (true);   // error: expected initializer before 'pre'
```

so the two spellings of what is nearly the same mistake behave differently.
(That diagnostic is itself a generic parse failure rather than a message
about contracts, which is a separate quality-of-implementation point.)


DISCOVERY

Found while comparing this test suite against an independent Clang contracts
implementation, which rejects every shape above.  The comparison was the
point: nothing in our own suite covered a contract in a typedef declaration,
because it had not occurred to anyone to write one.

Clang has the same gap, and there it is being fixed in the contracts branch.


ANALYSIS

The contract is parsed, attached to a real declarator node, and then dropped
because the declaration never becomes a function.

cp_parser_direct_declarator (gcc/cp/parser.cc) parses a
function-contract-specifier-seq in the CPP_OPEN_PAREN arm of its
direct-declarator loop -- that is, after ANY parameter list -- guarded only by
flag_contracts, and stores it on the cdk_function declarator node via
make_call_declarator.  It cannot do better: at that point it has not seen the
decl-specifiers, so `typedef` is invisible to it; it does not know whether it
is inside a parameter-declaration-clause; and it builds declarators inside-out,
so an enclosing pointer declarator has not been reached yet.

grokdeclarator (gcc/cp/decl.cc) then accumulates the node's specifiers into a
local in its `case cdk_function:` arm, and that local is READ only at the two
grokfndecl calls.  A typedef takes neither: the `typedef_p && decl_context !=
TYPENAME` block builds a TYPE_DECL and returns, and the local goes out of
scope unread.

The shape of the missing check is visible three lines away.  That same block
already contains

  if (reqs)
    error_at (location_of (reqs), "requires-clause on typedef");

for the requires-clause, which travels in the same declarator slot.  There is
no contract counterpart, and the same asymmetry appears at the type-id return
("requires-clause on type-id", which is why `using A = int (int) pre (true);`
is accepted too) and at the non-function-type case.

Two further details.  For a member typedef the predicate is never even
parsed: cp_parser_function_contract_specifier takes its deferred branch inside
a class being defined and stores a DEFERRED_PARSE token cache, and
cp_parser_late_contracts is only ever driven for a FUNCTION_DECL, so the
tokens are simply discarded.  And set_fn_contract_specifiers
(gcc/cp/contracts.cc) never asserts that its key is a FUNCTION_DECL, so
nothing downstream notices that no specifiers were ever recorded.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       accepted
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260914, b76fde4b175  yes

13.4.0, 14.4.0 and 15.3.0 do not accept the pre/post syntax at all.
````

## Reproducer

See [`contract-on-typedef-declaration.cpp`](contract-on-typedef-declaration.cpp)
in this directory.  No headers, so no `.ii` is needed.

## Notes

Fixed on this branch by `1250-contract-on-non-function-declarator`, which
closes this shape and four others in one check; the row survives here
because upstream still accepts it.

Related but separately tracked: [GCC-46](../gcc-46/gcc-46-contract-on-function-typed-parameter.md),
a contract on a parameter of function type, which is accepted the same way.
The two may well share a root cause -- both are declarators with a function
TYPE and no function DECLARATOR -- but that has not been established, and
they are kept apart until it is.  A maintainer who finds one fix covers both
should merge them rather than the reverse.

The watch test is
`gcc/testsuite/g++.dg/contracts/cpp26/open-bug-contract-on-function-typedef.C`,
which xfails this shape and GCC-46's together; the Clang mirror is
`clang/test/Contracts/OpenBugs/contract-on-function-typedef.cpp`.
