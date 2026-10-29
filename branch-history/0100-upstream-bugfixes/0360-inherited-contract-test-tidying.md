---
id: 0360-inherited-contract-test-tidying
subject: 'testsuite: contracts: tidy the contracts tests inherited from upstream'
depends: []
regenerates: []
fixes: []
---

## Rationale

The contracts tests upstream ships carry small defects of their own, none of
which changes what a test checks.  Four comments call a free function a
member function or name the wrong kind of contract
(`callerside-checks/freefunc-noexcept-{pre,post}.C`,
`outline-checks/freefunc-noexcept-{pre,post}.C`), and the tests in
`callerside-checks/` say nothing about what they test; each now has a
comment.  `intro.compliance.general.p2.3.4.C` is
`basic.contract.eval.p8-2.C` but for the paragraph its header quotes, so
the second now quotes both paragraphs and the first is gone.
`BZ121936-workaround-noipa.C` is the one test named with a `BZ` prefix and
uses `noinline`, not `noipa`; it becomes `pr121936.C`, with its typo
(`contract_asssert`) fixed and `dg-do` first.  The `dg-options` strings of
some thirty tests, and the `dg-additional-options` string of
`unused_warning.C`, end in a space or carry a doubled one; they no longer do.
And `throwing-violation-handler.cc`, a secondary source `dg.exp` never reads
as a test, loses a `dg-skip-if` that could never take effect (the test that
includes it, `contract-violation-noexcept2.C`, carries the guard).

The same spacing fix in a test this branch otherwise changes is part of the
commit that changes that test.

## Compile gap

None: test files only.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-4.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-5.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-SMF-post.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-SMF-pre.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-SMF2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-SMF3.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-SMF4.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p7.3.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p8-2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p8.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.scope.contract.p1.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.scope.contract.p2.1.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/callerside-checks/callerside-checks-non-trivial.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/callerside-checks/callerside-checks-none.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/callerside-checks/ctor.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/callerside-checks/freefunc-noexcept-post.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/callerside-checks/freefunc-noexcept-pre.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-warn-attributes.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-noexcept.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.res.p2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dependent_contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/intro.compliance.general.p2.3.4.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/name_mangling.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/outline-checks/freefunc-noexcept-post.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/outline-checks/freefunc-noexcept-pre.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/BZ121936-workaround-noipa.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr121936.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/src-loc-0.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/src-loc-1.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/src-loc-2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/throwing-violation-handler.cc : *
- gcc/testsuite/g++.dg/contracts/cpp26/unused_warning.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/vaargs.C : *
