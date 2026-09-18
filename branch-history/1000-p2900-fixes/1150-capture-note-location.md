---
id: 1150-capture-note-location
subject: "c++: contracts: don't read a location out of a non-DECL capture initializer"
depends: []
regenerates: []
fixes: [gcc-14]
---

## Rationale

GCC-14 (PR c++/126041).  When a lambda body names something a contract
predicate forbids, the diagnostic carries a note pointing at the offending
capture.  `cp_parser_lambda_body` produced that note's location with
`DECL_SOURCE_LOCATION` applied to the capture's initialiser, which is a
`DECL` only for the simplest captures.  For an init-capture, or a capture
whose initialiser is any other expression, the read produced garbage -- a
note pointing into an unrelated file and line, or at nothing at all.

The location is now taken from the initialiser only when it really is a
`DECL`, and falls back to the lambda's own location otherwise.  A reviewer
should check the fallback is the lambda and not the capture list, since a
default capture has no syntax of its own to point at.

## Compile gap

None: a self-contained change to how one note's location is computed.

## Contents

- gcc/cp/parser.cc : @cp_parser_lambda_body
- gcc/testsuite/g++.dg/contracts/cpp26/contract-capture-note-location.C : *
