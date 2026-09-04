# GCC-25: Result-name-introducer doesn't accept `identifier attribute-specifier-seq :`

**Status:** Fixed here (commit `49439509ad3`)
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

One limitation is deliberate and documented at the call site: an in-class
contract defers its predicate and builds its result variable later, from
the identifier stored on the contract node, which has nowhere to carry the
attributes -- so an attribute there is accepted and then silently
ignored. Accepted is the point; the attributes that make sense on a
result name are advisory. The extracted reproducer's own commentary
records this as a known limitation, not a residual bug.

## Notes

The reproducer was extracted from the fix commit's own DejaGnu test,
`gcc/testsuite/g++.dg/contracts/cpp26/pr125725.C` (added by gnu_gcc commit
`49439509ad3`), found via `git show 49439509ad3 --stat` and read with
`git show 49439509ad3:gcc/testsuite/g++.dg/contracts/cpp26/pr125725.C`.
DejaGnu directive lines were stripped, except the one `{ dg-warning ... }`
annotation (proving the `[[deprecated]]` attribute is actually applied,
not just parsed and dropped), which was replaced with a plain
`// expected-warning "..."` comment. The C++ source, including the test's
own commentary about the known in-class limitation, was kept as-is.

Measured directly against the stock binaries at
`/home/jberne4/repos/compilers/gcc-16.2.0/bin/g++` and `gcc-trunk/bin/g++`
(17.0.0 20260901): both reject the reported shape
(`post (r [[maybe_unused]]: r > 0)`) with the exact cascade described
above (`'r' was not declared in this scope`, followed by "two consecutive
'[' shall only introduce an attribute before '[' token") at
`-std=c++26 -fcontracts`, confirming the defect on both. Older stock
compilers (13.4.0, 14.4.0, 15.3.0) do not support C++26 contracts syntax
at all and are not applicable.
