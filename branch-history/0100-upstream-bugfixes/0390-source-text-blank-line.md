---
id: 0390-source-text-blank-line
subject: 'c++: source text of a contract predicate spanning a line of blanks'
depends: []
regenerates: []
fixes: [gcc-706]
---

## Rationale

GCC-706 (upstream: PR127857).  The comment of a contract is the source text
of its predicate, which `get_source_text_between` reads line by line.  On
each line after the first it skipped the leading blanks with `subspan`, and
on a line of nothing but blanks the offset equals the length, which trips
`char_span::subspan`'s assertion -- an ICE on valid code from every kind of
contract, release builds included.

A line after the first that is only blanks is now skipped, as an empty line
already was, and each line after the first is joined to the text with one
space in place of the line break and its leading blanks; a line with no
indentation used to be joined with none, so `sizeof` and `x` on two lines
read `sizeofx`.  The first line's check now also refuses a start column one
past the end of the line, which would reach the same assertion.

## Compile gap

None.  Everything named is upstream's.

## Contents

- gcc/input.cc : /line.length \(\) <= off/, /Replace the line break and any spaces at the beginning of later/, /^\t  if \(off == line.length \(\)\)$/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-comment-blank-line.C : *
