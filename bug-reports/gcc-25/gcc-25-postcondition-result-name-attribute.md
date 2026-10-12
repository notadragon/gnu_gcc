# GCC-25: Result-name-introducer doesn't accept `identifier attribute-specifier-seq :`

**Status:** Fixed here (commit [6d05ad4def05](https://github.com/notadragon/gnu_gcc/commit/6d05ad4def05f802b445aca9411c3136fbdaf358))
**Resolved by:** `1170-result-name-introducer`
**Component:** c++ / parser
**Upstream Link:** [PR125725](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125725)
**Affects:** stock g++ 16.2.0, trunk (17.0.0 20260901)

## Bug Report

A postcondition's result-name-introducer grammar is

```
result-name-introducer:
  identifier attribute-specifier-seq[opt] :
```

but only the plain `identifier :` spelling was recognized. An attribute
between the identifier and the colon, e.g.

```cpp
int f (int x) post (r [[maybe_unused]]: r > 0) { return x; }
```

was rejected: the parser fell through and treated `r [[maybe_unused]]:
r > 0` as the start of the predicate itself, producing a cascade of
unrelated errors beginning with `'r' was not declared in this scope`.

## Reproducer

See [`gcc-25-postcondition-result-name-attribute.C`](gcc-25-postcondition-result-name-attribute.C)
in this directory.

## Our Fix

`gcc/cp/parser.cc`: `cp_parser_contract_result_name` gained an `ATTRS`
out-parameter and now tentatively parses an attribute-specifier-seq after
the identifier; `cp_parser_function_contract_specifier` applies the
parsed attributes to the invented result variable. The parse is
tentative: an attribute can only follow the identifier here, so if what
comes after is not a colon, this was never a result-name-introducer and
the whole thing must go back to being parsed as a predicate -- the shape a
careless lookahead would break, which the test's `no_intro` control
covers.

An in-class contract defers its predicate and builds its result variable
later; the attributes travel with it, so an in-class
`post (r [[deprecated]]: ...)` warns on use just as an out-of-class one
does.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/pr125725.C` (added by [6d05ad4def05](https://github.com/notadragon/gnu_gcc/commit/6d05ad4def05f802b445aca9411c3136fbdaf358)).
DejaGnu directive lines were stripped, except the one `{ dg-warning ... }`
annotation (proving the `[[deprecated]]` attribute is actually applied,
not just parsed and dropped), which was replaced with a plain
`// expected-warning "..."` comment. The C++ source, including the test's
own commentary about the known in-class limitation, was kept as-is.

Measured directly against the stock binaries at
`gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`
(17.0.0 20260901): both reject the reported shape
(`post (r [[maybe_unused]]: r > 0)`) with the exact cascade described
above (`'r' was not declared in this scope`, followed by "two consecutive
'[' shall only introduce an attribute before '[' token") at
`-std=c++26 -fcontracts`, confirming the defect on both. Older stock
compilers (13.4.0, 14.4.0, 15.3.0) do not support C++26 contracts syntax
at all and are not applicable.

## Pending upstream comment (PR125725)

*Drafted for the user to post or drop; nothing here has been sent.  Checked
2026-09-25: no comment from berne@notadragon.com on PR125725.  Every claim
re-measured 2026-09-25 against stock `17.0.0 20260922` (the Compiler
Explorer nightly) and 16.2.0.  Attach [6d05ad4def05](https://github.com/notadragon/gnu_gcc/commit/6d05ad4def05f802b445aca9411c3136fbdaf358) and
`g++.dg/contracts/cpp26/pr125725.C`.*

~~~text
We have a patch.  cp_parser_contract_result_name now tentatively parses an
attribute-specifier-seq after the identifier, and
cp_parser_function_contract_specifier applies the attributes to the
result variable.  The parse has to be tentative: if what follows the
attributes is not a colon, this was never a result-name-introducer and the
whole thing must be reparsed as the predicate.

The attributes are applied, not just skipped over: with

  int f (int x) post (r [[deprecated]]: r > 0) { return x; }

the use of r in the predicate gets "'r' is deprecated", and the report's
one-line testcase compiles.
~~~
