# GCC-706: A contract predicate spanning a line of only blanks ICEs in `get_source_text_between`

**Status:** Fixed here
**Resolved by:** `0390-source-text-blank-line`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `ice-on-valid-code`
**Upstream Link:** [PR127857](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127857)
(filed 2026-10-12 by Yuancheng Jiang, UNCONFIRMED; not ours).
**Affects:** measured 2026-10-12 -- ICEs on `16.1.0`, `16.2.0`, the CE trunk
nightly `17.0.0 20261005` (`8802cd9fdbe`), the local trunk mirror
`17.0.0 20260922`, and this branch before the fix.  15.3.0 does not accept
the syntax.  Our Clang accepts it.

## Reproducer

[`gcc-706-contract-comment-blank-line.cpp`](gcc-706-contract-comment-blank-line.cpp),
at `-std=c++26 -fcontracts -fsyntax-only`; line 2 is two spaces:

    void f (int x) pre (x
      
      > 0);

    gcc-706-contract-comment-blank-line.cpp:3:5: internal compiler error: in subspan, at file-cache.h:52

An empty line 2 is fine.  The same ICE comes from a postcondition, from
`contract_assert`, from a member function's contracts (through
`cp_parser_late_contract_condition`), and from a predicate that begins with
an object-like macro defined above a line of blanks: the text range starts at
the macro's spelling, so it crosses that line (see GCC-707).  On this branch
the C front end's `_Pre`/`_Post`/`_ContractAssert` (P4299) ICE the same way,
from `c_build_contract_comment`, which calls the same function.

## Cause

`get_source_text_between` (`gcc/input.cc`), on the lines after the first,
counts the leading blanks and then takes `line.subspan (off, ...)`.  On a line
of nothing but blanks `off == line.length ()`, and `char_span::subspan`
asserts `offset < m_n_elts`.  An empty line never gets there: it is skipped
before.  It is a `gcc_assert`, so a release build ICEs too.

## Our Fix

`gcc/input.cc`: a line after the first that is only blanks is now skipped,
like an empty one, and every line after the first is joined with exactly one
space in place of the line break and its leading blanks.  Before, a
continuation line with no indentation was joined with none, so `sizeof` on
one line and `x` on the next read `sizeofx`.  The first line's bounds check
now also refuses a start column one past the end of the line, which would
reach the same assertion.

Tests: `g++.dg/contracts/cpp26/contract-comment-blank-line.C` (every kind of
contract, spaces, a tab, consecutive blank lines, an empty line, an unindented
continuation) and `gcc.dg/contracts/c-contract-comment-blank-line.c`.

## Found by

Reported upstream as PR127857, found by fuzzing.
