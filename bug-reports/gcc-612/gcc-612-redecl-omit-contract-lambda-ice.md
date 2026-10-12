# GCC-612: A templated function declared with a contract whose lambda captures a parameter, defined without repeating the contract -> ICE on instantiation

**Status:** Fixed here (commit [36d7bb4c0f3c](https://github.com/notadragon/gnu_gcc/commit/36d7bb4c0f3c7545506d1ea65b0e5a48bf3f3288)); latent upstream -- see Affects
**Resolved by:** `1260-outlined-lambda-capture-proxy`
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST
quicksearch, open and closed, on `contract lambda`, `contract lambda
template out-of-line`, `contract lambda capture template`, `contract
tsubst_expr`, `contracts lambda out-of-line`, `mismatched contract`.
Nearest: PR125712, the same omitted-contract shape without a lambda --
RESOLVED by `4d26b4cc14c` (on trunk and this branch), which is exactly the
path this case escapes; PR126038 / PR117435 / PR127287 are other lambda
crashes, already tracked as GCC-16 / GCC-31 / GCC-13)
**Affects:** measured 2026-09-26 -- this branch (installed 2026-09-26
02:52, `17.0.0 20260922`) ICEs, "in tsubst_expr, at cp/pt.cc:23546", when
the definition is instantiated.  Stock 16.1.0, 16.2.0 and trunk
`17.0.0 20260922` (`f008f03eff8`) never get that far: every member shape
segfaults while parsing the class on the in-class lambda alone (GCC-544,
fixed here), and every namespace-scope shape is rejected first with "use of
parameter outside function body" (GCC-31 / GCC-544c, fixed here).  So the
defect is upstream code, but no stock compiler can currently show it; it
is exposed by our GCC-544 and GCC-31 fixes
**Clang:** correct -- compiles, and the contract is checked on the
instantiated definition (both shapes, observe semantic, run test)
**Found:** 2026-09-26, while measuring a lambda repeated in a redeclaration's contract

## Bug Report

Not fileable as written today: stock reaches a different crash (GCC-544)
first, so the reproducer below shows only that one on any stock compiler.
File it after GCC-544 is filed (or fold it into that report as a second
case, "once the parse crash is fixed, instantiation crashes next"), and
re-measure stock then.  The draft below is written for that point.

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `17.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] ICE in tsubst_expr instantiating a definition that omits a contract whose lambda captures a parameter` |

Attachments:

| File | Description |
|---|---|
| [`redecl-omit-contract-lambda-ice.cpp`](redecl-omit-contract-lambda-ice.cpp) | Class template member declared with a capturing-lambda precondition, defined out of line without it |

````
A function template (or member of a class template) is first declared with
a precondition containing a lambda that captures a function parameter, and
later defined without repeating the contract, which [dcl.contract.func]
permits ("shall have either no function-contract-specifier-seq or the same
..." as the first declaration).  Instantiating the definition crashes.

```
template <class T> struct S { int f (int x) pre ([&] { return x > 0; } ()); };
template <class T> int S<T>::f (int x) { return x; }
template struct S<int>;
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only redecl-omit-contract-lambda-ice.cpp
redecl-omit-contract-lambda-ice.cpp: In instantiation of 'int S<T>::f(int) [with T = int]':
redecl-omit-contract-lambda-ice.cpp:15:17:   required from here
   15 | template struct S<int>;
      |                 ^~~~~~
redecl-omit-contract-lambda-ice.cpp:13:50: internal compiler error: in tsubst_expr, at cp/pt.cc:23546
   13 | template <class T> struct S { int f (int x) pre ([&] { return x > 0; } ()); };
      |                                                  ^~~~~~~~~~~~~~~~~~~~~
```

Expected: no diagnostic.

It needs a lambda that captures a parameter: [&], [=], [x] and [y = x] all
crash, while a captureless lambda ([] { return true; } (), or
[] (int y) { return y > 0; } (x)) and a [this] capture compile.  It needs
the contract to be omitted on a separate definition: the same contract on
an in-class definition compiles.  The member function template of a
non-template class and the namespace-scope function template crash the
same way:

```
template <class T> T fa (T x) pre ([&] { return x > 0; } ());
template <class T> T fa (T x) { return x; }
int main () { return fa (1) - 1; }
```

On the same compiler, the non-template forms of the same thing compile and
check the contract correctly.

For the column of the ICE: with [y = x] and [x] it points at the x in the
capture, with [&] at the lambda-introducer -- the implicit capture's
location -- so what is being substituted is the capture's initializer.


DISCOVERY

Found while testing redeclarations of functions whose contracts contain
lambdas.


ANALYSIS

From reading the code, not confirmed in a debugger.

The assertion is the PARM_DECL fallback in tsubst_expr (gcc/cp/pt.cc): a
parameter of a templated function, with no local specialization, reached
in an evaluated context -- gcc_assert (cp_unevaluated_operand).

When the definition is merged with the first declaration, duplicate_decls
calls update_contract_arguments (gcc/cp/contracts.cc), which rewrites the
first declaration's contracts in terms of the definition's parameters
(copy_and_remap_contracts -> remap_contract, a copy_tree_body_r walk over
the condition), and the definition's parameters become the pattern's
DECL_ARGUMENTS.  At instantiation, regenerate_decl_from_template takes the
pattern's contracts (the PR125712 fix) and tsubst_contract_specifier
registers the pattern's -- the definition's -- parameters as local
specializations.  Any reference to the first declaration's PARM_DECL that
the merge-time rewrite did not reach is therefore unregistered, and the
capture initializer of the lambda is one: tsubst_lambda_expr substitutes
it with tsubst_expr and hits the assertion.  Without a lambda every
parameter reference is in the condition proper and is rewritten, which is
why PR125712's shape now works.

Which link fails I could not settle by reading.  cp_walk_subtrees does
walk a LAMBDA_EXPR's capture initializers, so the rewrite should reach
them through the condition; that it evidently does not suggests the
lambda's capture list is not reached along the path the walk takes (or is
reached through a copy), or that the initializer is also held somewhere
the walk never visits, such as DECL_CAPTURED_VARIABLE of the capture
proxy or the closure's FIELD_DECL.  A breakpoint on the assertion, then
checking whether the PARM_DECL in LAMBDA_EXPR_CAPTURE_LIST of the
pattern's contract is the first declaration's or the definition's, would
decide it.  Either way the fix belongs at the merge (remap every
reference to the old parameters, including inside a lambda in the
condition), or at instantiation (also register the first declaration's
parameters when the contract came from it).


VERSIONS -- all on x86_64-linux-gnu

  source              version                       result
  compiler-explorer   16.1.0                        segfault parsing the class (other bug)
  compiler-explorer   16.2.0                        segfault parsing the class (other bug)
  compiler-explorer   17.0.0 20260922, f008f03eff8  segfault parsing the class (other bug)
  local build         17.0.0 20260922, parse crash fixed  ICE in tsubst_expr, pt.cc:23546
````

## Reproducer

See [`redecl-omit-contract-lambda-ice.cpp`](redecl-omit-contract-lambda-ice.cpp)
in this directory.  Syntax mode reaches it (the crash is in the front end,
at instantiation of the definition).

## Our Fix

`gcc/cp/contracts.cc`, `remap_contract`: the condition is walked with
`walk_tree`, which has no C++ language hook and so never enters a
`LAMBDA_EXPR`'s capture list; a capture initializer kept naming the first
declaration's parameter after the merge rewrote the rest of the condition in
terms of the definition's.  The lambdas in the condition are now collected
(`cp_walk_tree`) and their capture initializers remapped too.  (Walking the
whole condition with `cp_walk_tree` instead also remaps pack expansions and
breaks `dcl.contract.res.p1-pack.C`.)
Regression tests: `g++.dg/contracts/cpp26/redecl-omit-contract-lambda.C`
(`[&]`, `[y = x]`, `[x]`, `[=]` in a class template member, a member
template and a namespace-scope template, run with observe) and a templated
shape in `redecl-unnamed-param-lambda-capture.C`.

## Notes (ours, not for the report)

Shapes measured on the branch (installed compiler, `-c`): ICE for
`[&]`, `[=]`, `[x]`, `[y = x]` in a class template member; `[&]` in a
member function template of an ordinary class; `[&]` and `[=]` in a
namespace-scope function template.  Clean: `[]` with or without a
parameter passed in, `[y = 1]`, `[&] { return true; }` (captures nothing),
a lambda-local capture inside a captureless lambda, `[this]`, and an
in-class definition.  The non-template forms (`struct B { int h (int x)
pre ([&] { ... } ()); }; int B::h (int x) {...}` and the namespace-scope
function) compile and run correctly on the branch (observe: 0 violations
for 1, 2 for -1).

Repeating the contract on the definition is no way round it: the repeated lambda-expressions are not the same within one translation unit, which [dcl.contract.func] makes ill-formed, no diagnostic required, and GCC rejects it
("mismatched contract condition"); an unnamed parameter on the definition
(`int S<T>::f (int)`) ICEs the same way on GCC, and on Clang is rejected
"unnamed variable cannot be implicitly captured in a lambda expression"
(a Clang bug in its own right, not filed anywhere yet).

Stock masking: GCC-544 (member shapes) and GCC-31 / GCC-544c (namespace
shapes).  Both are fixed here, which is why the branch reaches this.  A
patch upstreaming the GCC-544 fix will expose this crash on stock, so it
should either carry the fix for this or say so.

Mirrored in the Clang fork by
`clang/test/Contracts/redecl-omit-contract-lambda.cpp` (run test).
