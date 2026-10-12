# GCC-495: A VLA named in a `contract_assert` inside a `[&]` lambda crashes the compiler

**Status:** Fixed here (commit [daed1aa78af1](https://github.com/notadragon/gnu_gcc/commit/daed1aa78af18cfdc357a7d7226e6495c30eb84c))
**Resolved by:** `1270-contract-only-capture-detection`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, summary
and comment text, on `contract_assert VLA`, `contract variable length
array`, `contract runtime bound`, `set_contract_capture_flag` and
`implicitly captured by a contract`; the hits, PR124648 (fixed: the plain
non-VLA form of the same diagnostic) and PR126041 (garbage note location),
are not this.  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0 and 16.2.0 segfault; stock trunk
`17.0.0 20260922` (`f008f03eff8`, checking enabled) fails the assertion in
`set_contract_capture_flag`; this branch segfaults
**Also:** the same program with `[=]` instead of `[&]` makes stock trunk
print the VLA by-copy error and then "confused by earlier errors, bailing
out" -- the same crash after an error (measured 2026-09-30, GCC-498); this
branch gives both errors and carries on.
**Clang:** correct -- "local entity 'a' is used only in contract assertions
and must be captured explicitly"
**Found:** 2026-09-25, review round #12 follow-up (`p3850impl/DECISIONS.md`
A12/G3: VLA support in both compilers is consistent enough to support)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] ICE in set_contract_capture_flag on a VLA named only in a contract_assert inside a [&] lambda` |

Attachments:

| File | Description |
|---|---|
| [`contract-assert-vla-implicit-capture-ice.cpp`](contract-assert-vla-implicit-capture-ice.cpp) | A VLA used only in a `contract_assert` in a default-capturing lambda: ICE |

````
A variable-length array (the GNU extension) that is named only in a
contract_assert inside a lambda with a capture-default crashes the
compiler.

```
void v (int n)
{
  int a[n];
  auto l = [&] { contract_assert (a[0] == 0); };
  l ();
}
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only \
    contract-assert-vla-implicit-capture-ice.cpp
contract-assert-vla-implicit-capture-ice.cpp: In lambda function:
contract-assert-vla-implicit-capture-ice.cpp:9:35: internal compiler error: in set_contract_capture_flag, at cp/semantics.cc:4705
    9 |   auto l = [&] { contract_assert (a[0] == 0); };
      |                                   ^
```

That is trunk, which is built with checking enabled; 16.1.0 and 16.2.0
report "internal compiler error: Segmentation fault" at the same location.

The program is ill-formed: [expr.prim.lambda.capture] forbids an implicit
capture whose only references are in contract assertions.  With int a[4]
instead of the VLA, GCC gives the expected

  error: 'a' is not implicitly captured by a contract assertion

and the same lambda with the VLA used in a plain return a[0] compiles, so
capturing a VLA by reference is otherwise supported.  Expected: the error
above for the VLA too.

A release-checking trunk build with -g shows where the segfault is:

```
0xa4f4d7 set_contract_capture_flag
0xa4f4d7 process_outer_var_ref(tree_node*, int, bool)
0xa58750 finish_id_expression_1
0xa58750 finish_id_expression(...)
0x9b7b81 cp_parser_primary_expression
```


DISCOVERY

Found while reviewing an implementation of C++26 contracts for how it
treats variable-length arrays.


ANALYSIS

From reading the code, consistent with the assertion above:
process_outer_var_ref (gcc/cp/semantics.cc) calls set_contract_capture_flag
right after add_default_capture whenever processing_contract_condition, and
set_contract_capture_flag asserts that the proxy's DECL_VALUE_EXPR is a
COMPONENT_REF of the capture FIELD_DECL.  For a VLA, build_capture_proxy
(gcc/cp/lambda.cc, the DECL_VLA_CAPTURE_P branch) rebuilds the array from
the {pointer, max index} capture struct and converts the pointer member, so
the value expression is not of that shape.  The capture FIELD_DECL needs to
be found some other way for a VLA, so that the flag is set and the usual
"not implicitly captured" error follows.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       result
  compiler-explorer   16.1.0                        ICE (segfault)
  compiler-explorer   16.2.0                        ICE (segfault)
  compiler-explorer   17.0.0 20260922, f008f03eff8  ICE (semantics.cc:4705)
  local build -g      17.0.0 20260922, 7aa4b1c5054  ICE (segfault)
````

## Our Fix

`gcc/cp/semantics.cc`, `set_contract_capture_flag`: look through the VLA proxy's value expression (and a `*this` proxy's ADDR_EXPR) for the COMPONENT_REF of the closure member instead of asserting it is that COMPONENT_REF; `gcc/cp/parser.cc`, `cp_parser_lambda_body`: recover the array from the capture's `{&a[0], bound}` initializer so the "not implicitly captured" error names `a`.  Regression test: `g++.dg/contracts/cpp26/contract-assert-vla-implicit-capture.C`.

## Notes (ours, not for the report)

Related, not this bug: with `[=]` instead of `[&]` stock trunk says "array of
runtime bound cannot be captured by copy, only by reference" and then
"confused by earlier errors, bailing out" -- on a release-checking build that
is itself a masked ICE after the error; not investigated.

Watch test: `g++.dg/contracts/cpp26/open-bug-contract-assert-vla-implicit-capture.C`
(`dg-ice`); Clang mirror
`clang/test/Contracts/OpenBugs/contract-assert-vla-implicit-capture.cpp`
(expected pass).

GCC-670 (ours, found 2026-10-07 writing GCC-669's mirrors, fixed with
them): once the crash was fixed, a VLA named in a `contract_assert` and then
outside it in the same `[&]` lambda (or in a lambda nested in it) was still
diagnosed, because name lookup finds the VLA's capture proxy itself and the
later use never reached `process_outer_var_ref`, which is where such a use
clears the mark.  On stock trunk the same shape is this bug's ICE.  Fixed in
`finish_id_expression_1`; cases `after`, `nested_after` and
`nested_inside_after` in `contract-assert-vla-implicit-capture.C`.
