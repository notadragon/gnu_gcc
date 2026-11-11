---
id: 6960-contracts-testsuite-readme
subject: 'testsuite: contracts: describe the conventions of g++.dg/contracts/cpp26'
depends: [6900-cross-paper-options]
regenerates: []
fixes: []
---

## Rationale

`g++.dg/contracts/cpp26/` holds over a thousand tests in several naming
families, with three subdirectories that have no `.exp` of their own,
shared JSON configuration files, extra translation units, and generated
`matrix-*` files, and until now nothing in the tree said which is which.
The new `README` records the naming families (draft stable names,
`pNNNN-*` per proposal, `prNNNNNN`, `open-bug-*` watch tests, `matrix-*`),
the two option conventions (`{ target c++26 }` with `dg-additional-options`
for most tests, `dg-options` with an explicit `-std=c++26` for most of the
sanitizer-routing ones), why a test pins `-fcontract-evaluation-semantic=`
even at its default, the `hostedlib` skip every contracts run test needs,
and how the `matrix-*` files are produced and why a hand edit to one does
not survive regeneration.

It lands after every per-paper entry because it describes all of them, and
after `6900-cross-paper-options` because it states that each
`-fcontracts-pNNNN` implies `-fcontracts`.

## Compile gap

None.  A text file that DejaGnu does not read.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/README : *
