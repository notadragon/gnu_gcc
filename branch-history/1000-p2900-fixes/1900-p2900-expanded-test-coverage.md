---
id: 1900-p2900-expanded-test-coverage
subject: 'c++: contracts: expand the base P2900 test coverage'
depends: []
regenerates: []
fixes: []
---

## Rationale

Twenty tests that demonstrate the base facility works, rather than pinning
any one bug fix.  They are the last commit of the band for a reason: the
band's milestone is "C++26 Contracts works with no known bugs that do not
require a new feature", and this is the commit that shows it.

Three kinds.

**Standard-wording citations.**  `basic.contract.eval.p14.C`,
`basic.contract.eval.p17-3.C`, `basic.contract.eval.p6.observe.C`,
`dcl.contract.res-auto-return-parm.C` and
`expr.prim.lambda.closure.p10.C` are named for the paragraph each one
exercises, so a reviewer can check the implementation against the wording
without reading the test first.

**Cross-product matrices.**  The seven `matrix-*` tests are generated from a
cross-product of the axes that interact: exception specifications, reader
contexts, and SFINAE, each in a compile, a codegen and an error variant.
They compile with plain `-fcontracts` and no paper flag, which is what makes
them base-facility tests and puts them in this band -- the paper-specific
variants of the same matrices (`matrix-sfinae-p4283*`,
`matrix-readers-p3097-codegen` and the rest) live with their papers.

**Behaviour coverage** for corners the wording citations do not reach:
reentrancy of a violation handler, contracts under SFINAE, the timing of
predicate instantiation, a firing postcondition on a `void` function, an
assertion inside a `noexcept` function with outlined checks, and a dependent
predicate that materializes a temporary.

Most of these have a counterpart in the companion Clang fork's test suite.
Holding the two implementations to the same behaviour is what makes each
side's gaps visible as tests the other side lacks.

## Compile gap

None.  It adds only tests, and every behaviour they exercise is established
by the base facility plus the fixes in `1010` through `1240`, all of which
precede it.

The matrix tests are the reason this commit is last in the band rather than
first: they exercise the facility as a whole, so running them before the
per-bug fixes would report those bugs as failures rather than as fixed.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p14.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p17-3.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic.contract.eval.p6.observe.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-multi-observe.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-constexpr-observe-cap.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-dependent-predicate-temporary.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-reentrancy-basic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.res-auto-return-parm.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/expr.prim.lambda.closure.p10.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-exceptspec-errors.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-exceptspec.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-readers-codegen.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-readers.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-sfinae-codegen-errors.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-sfinae-errors.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/matrix-sfinae.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/outline-checks/func-noexcept-assert.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/predicate-instantiation-timing.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/sfinae-contracts.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/void-postcondition-firing.C : *
