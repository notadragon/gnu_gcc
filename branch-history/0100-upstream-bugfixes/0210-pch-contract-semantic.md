---
id: 0210-pch-contract-semantic
subject: 'c-family: a PCH must match the contract evaluation semantic'
depends: []
regenerates: []
fixes: [gcc-510]
---

## Rationale

GCC-510.  The contract evaluation semantic is resolved while a header is
parsed and saved in its contracts, so a precompiled header built with
`-fcontract-evaluation-semantic=ignore` and used by a translation unit that
asks for `enforce` silently made the header's contracts ignored -- at run
time and in constant evaluation -- and `-Winvalid-pch` said nothing.

`-fcontract-evaluation-semantic=` joins `-fexceptions` in `pch_matching`, so
such a PCH is not used (with `-Winvalid-pch`, "settings for
-fcontract-evaluation-semantic= do not match") and the header is read as
source.

## Compile gap

None.  `flag_contract_evaluation_semantic` is upstream's option variable.

## Contents

- gcc/c-family/c-pch.cc : /flag_contract_evaluation_semantic/
- gcc/testsuite/g++.dg/pch/contract-semantic-mismatch.C : *
- gcc/testsuite/g++.dg/pch/contract-semantic-mismatch.Hs : *
