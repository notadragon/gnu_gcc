---
id: 1210-deferred-friend-redeclaration-limit
subject: 'c++: contracts: record the deferred-redeclaration matching limitation'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not a fix -- a defect written down and pinned, because the code said `TODO:
ignore these and figure out how to process them later` and nothing said what
went wrong if you did not.

`check_redecl_contract` skips redeclaration contract matching when either side
still has `DEFERRED_PARSE` contracts at the merge point.  That happens for a
friend declaration, whose contracts are late-parsed at the end of the class,
while the same function declared outside the class definition is not deferred.
The match is skipped and is never re-run once the contracts are late-parsed,
so a contract *mismatch* between two such declarations -- two friend
declarations of the same function with different predicates -- is silently
accepted, unlike every non-deferred redeclaration path.

The comment says all of that, points at the xfail test, and says what closing
it would take (queuing the deferred contracts and comparing them after
late-parse).  `contract-friend-deferred-mismatch.C` is the watch test; it
xfails here and is the reproducer behind GCC-27 (PR c++/127291), which is open
both upstream and here.

## Compile gap

None: a comment and an xfail test.

## Contents

- gcc/cp/contracts.cc : @check_redecl_contract
- gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C : *
