---
id: 1900-p2900-expanded-test-coverage
subject: 'c++: contracts: P2900: expand the base-facility test coverage'
depends: []
regenerates: []
fixes: []
---

## Rationale

Twenty-two tests that demonstrate the base facility works, rather than
pinning any one bug fix.  They are the last commit of the band for a reason:
the band's milestone is "C++26 Contracts works with no known bugs that do not
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

**Declarator positions**, added 2026-09-16.
`contract-declarator-positions.C` covers the declarators a contract may NOT
occupy but which have a function TYPE -- a name declared through a typedef or
alias of a function type -- and
`this-in-out-of-line-member-contract.C` covers the one it may, with `this`
named in the predicate of an out-of-line member definition.  The second is
here rather than beside a fix because GCC is correct on it and the companion
Clang fork is not; it exists so that fixing Clang cannot be mirrored into a
regression here.

**Behaviour coverage** for corners the wording citations do not reach:
reentrancy of a violation handler, contracts under SFINAE, the timing of
predicate instantiation, a firing postcondition on a `void` function, an
assertion inside a `noexcept` function with outlined checks, a dependent
predicate that materializes a temporary, and a predicate that throws only
through the destructor a delete-expression invokes or through an initializer
list's array filler, and a fold over a class template's pack in the
precondition or postcondition of a member declared in the class and defined
outside it, and the contract assertions of a destructor evaluated in a
variable's constant destruction (a `constexpr` variable or a class-type
template argument is ill-formed under `enforce` and warned under `observe`;
any other object is destroyed at exit instead).

Upstream's `contracts-friend1.C` becomes a run test: it carried `dg-output`
lines for the four violations it provokes but was compile-only, so they were
never checked.

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
- gcc/testsuite/g++.dg/contracts/cpp26/contract-declarator-positions.C : *
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
- gcc/testsuite/g++.dg/contracts/cpp26/this-in-out-of-line-member-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/void-postcondition-firing.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/predicate-throw-delete.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/predicate-throw-array-filler.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-cdtor-contracts.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-dtor-contracts-constant-destruction.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-dtor-contracts-constant-destruction-observe.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-observe-fold-drops-runtime-check.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-observe-nonconstant-predicate.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-post-reference-result.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-result-name-subobject.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constify-binding-ref-nttp.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constify-requires-expr.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-cxx98.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-ref-capture-constify.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/ctor-pre-member-in-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/ctor-pre-static-overload.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/default-outofline-adds-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/default-outofline-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dtor-contracts-base-alias.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dtor-contracts-per-variant.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/dtor-redecl-contracts-not-compared.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/extension-spelling-without-contracts.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contracts-friend1.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-attributes-ignored-known.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic-contract-eval-example-enforce.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/basic-contract-eval-example-ignore.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-nested.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/local-class-in-predicate-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/nested-assert-in-stmt-expr.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/observe-warning-unused-var-eval.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pch-checked-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/post-capture-kind-on-predicate.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/post-odr-use-template-shapes.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-outer-local.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/predicate-throw-not-detected.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/redecl-condition-folded.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/redecl-post-param-const.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/redecl-result-name-presence.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-binding-lambda-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-binding-lambda-ref-constify.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/generic-lambda-free-function-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-in-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-shadows-parameter.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-callee-precondition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-volatile-read.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-weak-callee-2.cc : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-weak-callee.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/retval-leak-throwing-post.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/self-name-in-own-contract.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-constified.C : *
- gcc/testsuite/g++.dg/modules/contract-inline-checked_a.C : *
- gcc/testsuite/g++.dg/modules/contract-inline-checked_b.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/ce-assume-semantic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-copy-capture-constify.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/redecl-omit-contract-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-object-lifetime.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/constexpr-postcondition-param-address.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture-edges.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-declaration-entities-constexpr.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-declaration-entities-errors.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-declaration-entities.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-param-cv-member-definition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-typeid-polymorphic-param.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-unnamed-param-redecl.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/postcondition-unnamed-param-definition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pre-without-parenthesis.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/predicate-exception-quick-enforce.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/qualified-friend-member-contract-mismatch.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-deduced-run.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-deduced.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-init-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-lambda-enclosing-scope.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/this-param-out-of-class-quick.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/this-param-out-of-class-run.C : *
- gcc/testsuite/g++.dg/pch/contract-constexpr.C : *
- gcc/testsuite/g++.dg/pch/contract-constexpr.Hs : *
- gcc/testsuite/g++.dg/pch/contract-runtime.C : *
- gcc/testsuite/g++.dg/pch/contract-runtime.Hs : *
- gcc/testsuite/g++.dg/contracts/cpp26/codegen-debug-info.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-classtemplate-pack-member-fold.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-classtemplate-pack-member-fold-constexpr.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-repeated-location.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-repeated-location-constexpr.C : *
