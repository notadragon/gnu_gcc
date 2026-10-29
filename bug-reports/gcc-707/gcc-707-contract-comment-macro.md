# GCC-707: A contract predicate that begins or ends inside a macro expansion gets a wrong comment

**Status:** Fixed here
**Resolved by:** `1400-contract-comment-macro`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** None found (searched 2026-10-12: `contract macro comment`,
`contract violation comment macro`, `contracts source text macro`,
`get_source_text_between`, and the open c++ PRs with "contract" in the
summary; only PR127857, which mentions it in its notes, and PR125978, an
over-verbose comment from the pretty-printed fallback, are near it).
**Affects:** measured 2026-10-12 -- the same five comments on `16.1.0`,
`16.2.0`, the CE trunk nightly `17.0.0 20261005` (`8802cd9fdbe`), the local
trunk mirror `17.0.0 20260922`, and this branch before the fix.  15.3.0 does not accept the syntax.  Our Clang prints
every predicate as written; the EDG front end's own comment is the predicate's
tokens after macro expansion (see "Our other compilers").

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] the comment of a contract whose predicate begins or ends in a macro expansion is wrong, and can include unrelated source` |

Attachments:

| File | Description |
|---|---|
| [`gcc-707-contract-comment-macro.cpp`](gcc-707-contract-comment-macro.cpp) | Prints the five comments under `observe` |

````
The comment of a contract violation is meant to be the source text of the
predicate.  When the predicate begins or ends inside a macro expansion, the
comment is something else -- and in one shape it is all the source between
the macro's definition and the contract.

```
#include <contracts>

#define M 0
#define LT(a, b) ((a) < (b))
#define X x
#define ZERO 0
#define ID(e) e

// Each violation's comment should be its predicate as written.
int f (int x) pre (M > x) { return x; }
int g (int x) pre (LT (x, 0)) { return x; }
int h (int x) pre (x < ZERO) { return x; }
int i (int x) pre (X < ZERO) { return x; }
int j (int x) pre (ID (x) < 0) { return x; }

int main ()
{
  f (1); g (1); h (1); i (1); j (1);
}
```

```
$ g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe \
      gcc-707-contract-comment-macro.cpp && ./a.out 2>&1 | grep -o 'cpp:.*'
cpp:10: 0#define LT(a, b) ((a) < (b))#define X x#define ZERO 0#define ID(e) e// Each violation's comment should be its predicate as written.int f (int x) pre (M > x
cpp:11: ((a) < (b))
cpp:12: (((int)((const int)x)) < 0)
cpp:13: x#define ZERO 0
cpp:14: x) < 0
```

Expected: M > x, LT (x, 0), x < ZERO, X < ZERO, ID (x) < 0.

  f  begins in a macro, ends outside it: every source line from the macro's
     definition to the end of the predicate, other definitions and comments
     included.  With the macro defined further away, the comment is
     correspondingly longer, and contains whatever source lies between.
  g  one expansion of a function-like macro: its replacement list.
  h  begins outside, ends in a macro defined above it: the text range runs
     backwards, so the comment falls back to the printed expression.
  i  begins in one macro, ends in another defined below it: the part of the
     #define lines between the two.
  j  begins in a macro argument: the macro name and its open parenthesis are
     lost.

A macro defined in another file (a header) also falls back to the printed
expression, since the two ends are in different files.  The program is
otherwise correct: only the text reported by contract_violation::comment()
is wrong.


DISCOVERY

The notes of PR127857, whose second reproducer is a predicate beginning with
an object-like macro defined above a line of blanks: there the range crosses
that line and ICEs.  With the ICE out of the way, the same range produces
shape f above; the rest came from probing where else the ends can land.


ANALYSIS

build_comment (gcc/cp/contracts.cc) passes the start and finish of the
condition's location to get_source_text_between (gcc/input.cc), which
expands each with expand_location_to_spelling_point.  For a token that came
from a macro expansion, that is the token's place in the macro definition
(or, for a macro argument, in the argument).  The two ends are resolved
independently and the text between them is returned whenever both are in
the same file and in order, so the range can begin in a #define and end in
the contract, or begin and end in two different #define lines.  When they
are out of order or in different files the function returns NULL and
build_comment pretty-prints the expression instead.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       comments
  compiler-explorer   15.3.0                        (no contracts)
  compiler-explorer   16.1.0                        as above
  compiler-explorer   16.2.0                        as above
  compiler-explorer   17.0.0 20261005, 8802cd9fdbe  as above
  local build -g      17.0.0 20260922               as above
````

## Reproducer

See [`gcc-707-contract-comment-macro.cpp`](gcc-707-contract-comment-macro.cpp)
in this directory.  It has an `#include <contracts>`, so a `.ii` should
accompany any Bugzilla attachment.

This branch prints the five predicates as written (see Our Fix).

## The wider probe (2026-10-12)

Sixteen shapes, measured on stock trunk, this branch, our Clang, and the EDG
front end.  `X` is `x`, `ZERO` is `0`, `Y` is `X`, `NEG(a)` is `((a) < 0)`,
`ID(e)` is `e`, `LHS` is `x <`, `RHS` is `< 0`, `HX` is `x` from a header.

| Predicate | GCC | Clang | EDG's own comment |
|---|---|---|---|
| `X < 0`, `LHS 0`, `Y < 0`, `NEG (x) && x > 9` (begin in a macro, end outside) | source from the `#define` through the predicate | as written | expanded tokens |
| `x < ZERO`, `x RHS`, `x > 9 \|\| NEG (x)` (end in a macro) | printed expression | as written | expanded tokens |
| `X < ZERO`, and `contract_assert (X < ZERO)` | `x#define ZERO 0` | as written | `x < 0` |
| `NEG (x)`, `NEG (x) \|\| NEG (x)` | `((a) < 0)` | as written | expanded tokens |
| `NEG (x) \|\| X < ZERO` (begins in a later `#define` than it ends) | printed expression | as written | expanded tokens |
| `HX < 0` (macro from a header) | printed expression | as written | `x < 0` |
| `ID (x) < 0` | `x) < 0` | as written | `x < 0` |
| `ID (x < 0)` (whole predicate a macro argument) | `x < 0` | as written | `x < 0` |
| `PRE_NEG`, a macro expanding to `pre (x < 0)` | `x < 0` | `PRE_NEG` | `x < 0` |

## Our other compilers

Clang reads the text of the predicate's expansion range, so the comment is
the predicate as written in every shape (for `PRE_NEG` that is the macro's
name, since the whole contract is the expansion).  The EDG front end forms
its comment from the predicate's tokens after macro expansion
(`scan_contract_predicate`, `expr.c`), so it is consistent but expanded.
Through `edg-gxx`, which is how EDG programs run, the comment is g++'s
printed expression for every contract, macros or not: `edg_gxx.sh` passes
`-fcontract-comment-from-expression` because the generated C++'s columns
are not the original source's.

## Our Fix

The comment is now formed from the predicate's tokens and the location of
the token that follows it, by `get_source_text_of_tokens` (`gcc/input.cc`),
which both front ends call: the C++ parser at its three contract parses
(`cp_parser_contract_comment`, passed to `grok_contract` and
`update_late_contract`), and the C parser's `_Pre`, `_Post` and
`_ContractAssert`.

The predicate is lifted out of macro expansions to the context where the
contract's own parentheses were written, a token from an expansion within
that context becoming the macro's name as invoked there.

* Outside any expansion, the comment is the source text from the
  predicate's first token to its last, extended to the closing parenthesis
  of a function-like macro invocation that the last token comes from
  (found by scanning the source past the name, skipping comments and
  string and character literals).  So `NEG (x)`, `X < ZERO` and
  `ID (x) < 0` are their own comments.
* Inside a macro, it is the text of the replacement list when every token
  of the predicate is from it (`PRE_NEG` gives `x < 0`), and the text of an
  argument as written, recursively, when every token is from one use of
  one argument (`MY_PRE (X < ZERO)` gives `X < ZERO`).
* Otherwise -- tokens from both, from two arguments or two uses of one --
  and always with `-ftrack-macro-expansion=0`, it is the predicate's
  tokens: names and punctuators as such, literals as written (from their
  values when macro locations are not tracked), separated by a space where
  whitespace preceded a token or two tokens were not written next to each
  other.  `CHK (-x)` with `contract_assert ((c) > 0)` gives `(-x) > 0`.

`-fcontract-comment-from-expression` still prints the expression instead.
Unlike Clang, a contract written by a wrapper macro gets the predicate
(`x < 0`), not the invocation (`MY_PRE (x < 0)`).

Tests: `g++.dg/contracts/cpp26/contract-comment-macro.C` (every shape above
for each kind of contract, a member function, a macro from a header, a
multi-line invocation, a comment and a string in one),
`contract-comment-macro-untracked.C` (`-ftrack-macro-expansion=0`), and
`gcc.dg/contracts/c-contract-comment-macro.c`.
