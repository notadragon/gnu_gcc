# GCC-544: A lambda in the in-class contract of a templated member function -> ICE

**Kind:** defect (ice-on-valid-code)
**Status:** Fixed here (commit [de4538e04341](https://github.com/notadragon/gnu_gcc/commit/de4538e0434118f629564a09cc2157e27abc81ec))
**Resolved by:** `1290-lambda-in-declaration-contract`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `pushtag`, `begin_lambda_type`, `add_stmt contract`,
`lambda class template contract`, `lambda precondition template`, plus every
open bug with `contract` in its summary; PR127287 and PR126039 involve
lambdas in contracts but are different crashes, and PR127428 (GCC-543) is a
different crash from the same deferred parse of a member's contract.
Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` (`f008f03eff8`) crash on a lambda in the in-class
contract of a member function of a class template, or of a member function
template.  Reduced 2026-09-26: the lambda need not capture, the function
need not have parameters, and a definition crashes the same as a
declaration (earlier notes here said a definition compiled; that was wrong).
**Workaround:** name a helper function instead of writing a lambda
**Clang:** compiles all three shapes (branch; stock Clang has no contracts)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] ICE in add_stmt on a lambda in the contract of a member function of a class template` |

Attachments:

| File | Description |
|---|---|
| [`class-template-member-decl-contract-lambda-ice.cpp`](class-template-member-decl-contract-lambda-ice.cpp) | Member function of a class template |
| [`template-member-decl-contract-lambda-ice.cpp`](template-member-decl-contract-lambda-ice.cpp) | Member function template of an ordinary class |

````
A lambda-expression in the contract of a member function of a class
template crashes the compiler while the class is parsed; no instantiation
is needed.

```
template <class T> struct S
{
  void f () pre ([] { return true; } ());
};
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    class-template-member-decl-contract-lambda-ice.cpp
class-template-member-decl-contract-lambda-ice.cpp: In member function 'void S<T>::f()':
class-template-member-decl-contract-lambda-ice.cpp:11:19: internal compiler error: Segmentation fault
   11 |   void f () pre ([] { return true; } ());
      |                   ^
```

A member function template of an ordinary class crashes the same way:

```
struct S
{
  template <class T> void f () pre ([] { return true; } ());
};
```

```
template-member-decl-contract-lambda-ice.cpp:11:38: internal compiler error: Segmentation fault
   11 |   template <class T> void f () pre ([] { return true; } ());
      |                                      ^
```

It makes no difference whether f is only declared or also defined in the
class, whether the contract is a precondition or a postcondition, whether
f is static, or what the lambda captures ([t] or [&] naming a parameter
crash identically).  A lambda in a contract_assert in the body of f is
fine, and the same member of a non-template class does not crash here
(though it leaves state behind that crashes later: PR127428).

A release-checking trunk build with -g shows the crash:

```
0xa49593 vec<tree_node*, va_gc, vl_embed>::last()
0xa49593 add_stmt(tree_node*)
0x97fee5 pushtag(tree_node*, tree_node*, TAG_how)
0x8cb6ad xref_tag(tag_types, tree_node*, TAG_how, bool)
0x913ba2 begin_lambda_type(tree_node*)
0x9d3dcb cp_parser_lambda_expression
...
0x99b71c cp_parser_late_contract_condition(cp_parser*, tree_node*, tree_node*)
```


DISCOVERY

Found while reviewing an implementation of C++26 contracts for lambdas in
contract predicates.


ANALYSIS

From reading the code, consistent with the trace: in a template, pushtag
records the closure type as a local class with a DECL_EXPR via add_stmt,
but a member's contract is parsed late (cp_parser_late_contract_condition,
from the class's late-parse loop in cp_parser_class_specifier) with no
statement list open, so add_stmt reads the last element of an empty
statement-list stack.  Either the late parse needs a statement list (or a
proper function context), or the closure must not be recorded as a local
tag there.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       ICE
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260922, f008f03eff8  yes
  local build -g      17.0.0 20260922, 7aa4b1c5054  yes
````

## Our Fix

`gcc/cp/name-lookup.cc`, `pushtag`: add the local class's DECL_EXPR only when a statement list is open.  There is none while a contract on a declaration is parsed, and the closure of a lambda in it is instantiated with the contract's LAMBDA_EXPR, so it needs none.  Regression tests: `template-decl-contract-lambda.C`, `-class.C`, `-nsscope.C`.

Upstreaming this fix exposes GCC-612 on stock (instantiating a definition
that omits such a contract ICEs in `tsubst_expr`); its fix, remapping the
lambdas' capture initializers in `remap_contract`, should travel with it.

## Notes (ours, not for the report)

The third reproducer,
[`template-decl-contract-lambda-nsscope.cpp`](template-decl-contract-lambda-nsscope.cpp)
(a namespace-scope function template declaration), is not part of the
report: stock rejects it with "use of parameter outside function body",
which is PR117435 (a lambda in a contract cannot capture a parameter; stock
rejects the non-template `void fa (int t) pre ([&] { return t > 0; } ());`
the same way).  This branch, which fixed PR117435 and defers every
predicate, crashes on it instead -- also in the declaration replay in
`cp_parser_init_declarator` (`gcc/cp/parser.cc`).  Same family as GCC-543
(the replay runs without a function context).

Watch tests: `g++.dg/contracts/cpp26/open-bug-template-decl-contract-lambda.C`
(member template), `open-bug-template-decl-contract-lambda-class.C` (class
template member) and `open-bug-template-decl-contract-lambda-nsscope.C`
(namespace scope), all `dg-ice`; Clang mirror (expected pass)
`clang/test/Contracts/OpenBugs/template-decl-contract-lambda.cpp`.
