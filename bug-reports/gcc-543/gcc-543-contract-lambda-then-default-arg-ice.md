# GCC-543: A lambda in a declaration's contract, then any default argument -> ICE

**Kind:** defect (ice-on-valid-code)
**Status:** Fixed here (commit [4cce0358680d](https://github.com/notadragon/gnu_gcc/commit/4cce0358680d279367a28a88317a3f85f4a35a3c))
**Resolved by:** `1290-lambda-in-declaration-contract`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** [PR127428](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127428)
+ comment pending (UNCONFIRMED, filed 2026-09-16 by someone else: a
constructor declaration with a lambda in `pre`, then a deducing-this member
template, segfaults in `poplevel`).  Confirmed the same defect 2026-09-26:
reduced, both reproducers are "a member function declaration whose
precondition contains a lambda", followed by any template declaration
(PR127428's crash, in `poplevel`) or any default argument (ours, in
`cp_parser_default_argument`), and at both crash sites `cfun` is left
non-null with no language data.  Found by Bugzilla REST search (summary and
comment text) on `precondition lambda constructor`; `contract default
argument` and every open bug with `contract` in its summary show nothing
else.
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` (`f008f03eff8`) crash on the in-class shape.  This branch
crashes on it and also on the namespace-scope shape, which stock compiles.
**Workaround:** move the lambda out of the predicate (name a function
instead), or make the first declaration a definition
**Clang:** compiles both shapes (branch; stock Clang has no contracts)

## Bug Report

Already filed as PR127428, by someone else, with a larger testcase
(`consteval` lambda, `if consteval`, `<string_view>`, a deducing-this member
template, an out-of-line constructor definition) and a crash in `poplevel`.
Nothing new to file; the useful contribution is a confirming comment with
the reduction, the second crash site, and what the debugger shows.

| File | Description |
|---|---|
| [`contract-lambda-then-default-arg-ice.cpp`](contract-lambda-then-default-arg-ice.cpp) | Member declaration with a lambda in `pre`, then `void f4 (int y = 1);` |

## Pending upstream comment (PR127428)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-26: PR127428 has only its opening comment.  Re-measured 2026-09-26
against 16.1.0, 16.2.0, stock trunk `17.0.0 20260922` (`f008f03eff8`, the
Compiler Explorer nightly) and a release-checking trunk build with -g
(`7aa4b1c5054`), which gave the traces and the debugger state.*

~~~text
Confirmed, and it reduces a long way.  Only the lambda in the precondition
of a member function declaration matters; consteval, if consteval,
<string_view>, deducing this and the out-of-line constructor definition
can all go.  Any later template declaration crashes:

  struct A { void f (int x) pre ([] { return true; } ()); };
  template <class T> struct B { };

  $ g++ -std=c++26 -fcontracts -fsyntax-only t.cpp
  t.cpp:2:32: internal compiler error: Segmentation fault
  0x8deed1 poplevel(int, int, int)
  0x9ec6c8 end_template_decl()
  0x9d66f8 cp_parser_template_declaration_after_parameters

and so does any later default argument, at a different site:

  struct A { void f (int x) pre ([] { return true; } ()); };
  void g (int y = 1);

  t.cpp:2:13: internal compiler error: Segmentation fault
  0x9bdfb3 cp_parser_default_argument
  0x9bdfb3 cp_parser_parameter_declaration
  0x9be56f cp_parser_parameter_declaration_list

Both crash on 16.1.0, 16.2.0 and trunk (17.0.0 20260922); the capture
does not matter ([x] { return x > 0; } crashes the same way).  There is no
crash if f is defined rather than only declared, or if the declaration is
at namespace scope.

At the crash, in both cases, cfun is non-null but cfun->language and
cfun->decl are null, and current_function_decl is null.  Both sites
dereference cp_function_chain behind an "if (cfun)" test (poplevel via
named_labels, cp_parser_default_argument directly), so it looks like
parsing the lambda in a member declaration's deferred contract leaves a
stale cfun behind, and whatever next tests cfun falls over.
~~~

## Our Fix

`gcc/cp/parser.cc`, `cp_parser_lambda_body`: when there was no `cfun` before the lambda (a contract on a declaration is parsed with `current_function_decl` set but no body), restore `cfun` to NULL and `current_function_decl` to the function after `pop_function_context`, which otherwise left the placeholder `push_function_context` saved.  Regression tests: `decl-contract-lambda-default-arg.C`, `decl-contract-lambda-default-arg-nsscope.C`.

## Notes (ours, not for the report)

The branch defers every contract predicate, so the namespace-scope shape in
[`contract-lambda-then-default-arg-nsscope-ice.cpp`](contract-lambda-then-default-arg-nsscope-ice.cpp)
crashes too (stock compiles it).  Cause: the declaration replay in
`cp_parser_init_declarator` (`gcc/cp/parser.cc`: `inject_parm_decls` +
`cp_parser_late_contracts` under `current_function_decl = fn`), and the class
late-parse loop over `unparsed_contracts` in
`cp_parser_class_specifier` (the stock path), both parse the predicate with
`current_function_decl` set but no function context.  The lambda's
push/pop of function context against a bare declaration leaves state (cfun /
function context) that the next `cp_parser_parameter_declaration` trips
over.  Fix direction: run the replay inside a proper function-context
sentinel, as `start_function` gives the definition path.  Clang compiles
both shapes (branch).

Watch tests: `g++.dg/contracts/cpp26/open-bug-decl-contract-lambda-default-arg.C`
(in-class) and `open-bug-decl-contract-lambda-default-arg-nsscope.C`
(`dg-ice`); Clang mirror (expected pass)
`clang/test/Contracts/OpenBugs/decl-contract-lambda-default-arg.cpp`.
