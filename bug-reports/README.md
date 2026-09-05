# Open Upstream Bugs

Bugs found during this implementation that reproduce on stock upstream GCC,
independent of anything in this branch. Each links to a self-contained
report-ready writeup plus a reproducer. A row is removed (and its file
deleted) once the bug is fixed on upstream master, regardless of who fixed
it or whether it was ever formally filed.

| Bug | Summary | Status | Upstream Link | Details |
|-----|---------|--------|----------------|---------|
| GCC-1 | Contract on a function with a variadic parameter pack ICEs or misattributes a diagnostic | Fixed here | [PR124395](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124395) | [gcc-01-contract-pack-ice.md](gcc-01-contract-pack-ice.md) |
| GCC-2 | Constant evaluator does not diagnose forming an address into an object before its constructor begins, for a virtual base | Open | -- | [gcc-02-constexpr-vbase-before-ctor.md](gcc-02-constexpr-vbase-before-ctor.md) |
| GCC-3 | A contract condition that re-calls a constexpr function already called in the same constant evaluation is wrongly rejected as non-constant | Fixed here | [PR125459](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125459) | [gcc-03-constexpr-repeat-call.md](gcc-03-constexpr-repeat-call.md) |
| GCC-5 | Contract on a function returning a non-trivially-destructible class double-destroys the return value | Fixed here | -- | [gcc-05-contract-retval-double-destroy.md](gcc-05-contract-retval-double-destroy.md) |
| GCC-9 | Nested `[[assume]]` leaks the inner operand's side effects during constant evaluation | Fixed here | -- | [gcc-09-nested-assume-side-effect-leak.md](gcc-09-nested-assume-side-effect-leak.md) |
| GCC-10 | Contract predicate on a lambda capturing `this` reads the closure object as the enclosing class | Fixed here | -- | [gcc-10-lambda-this-capture-in-contract.md](gcc-10-lambda-this-capture-in-contract.md) |
| GCC-11 | Lambda in a non-member postcondition with a result name fails to parse | Fixed here | -- | [gcc-11-lambda-in-postcondition-result-name.md](gcc-11-lambda-in-postcondition-result-name.md) |
| GCC-12 | A postcondition's result binding denotes two different objects within one predicate evaluation | Fixed here | [PR112794](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112794) | [gcc-12-result-binding-denotes-two-objects.md](gcc-12-result-binding-denotes-two-objects.md) |
| GCC-13 | Lambda's own contract specifiers are never substituted when it's instantiated | Fixed here | -- | [gcc-13-precondition-in-lambda-nested-in-generic-lambda.md](gcc-13-precondition-in-lambda-nested-in-generic-lambda.md) |
| GCC-14 | Garbage source location on a contract-capture diagnostic note | Fixed here | [PR126041](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126041) | [gcc-14-contract-capture-note-garbage-location.md](gcc-14-contract-capture-note-garbage-location.md) |
| GCC-15 | Outlined contract checks lose by-value parameter/result mutations | Fixed here | -- | [gcc-15-outlined-checks-lose-by-value-mutations.md](gcc-15-outlined-checks-lose-by-value-mutations.md) |
| GCC-16 | Contract on a capturing lambda inside an instantiated template segfaults | Fixed here | -- | [gcc-16-lambda-capture-contract-in-template.md](gcc-16-lambda-capture-contract-in-template.md) |
| GCC-17 | `this` accepted in the trailing return type of an explicit-object member function | Open | -- | [gcc-17-this-in-xobj-declaration.md](gcc-17-this-in-xobj-declaration.md) |
| GCC-18 | A predicate lambda naming a namespace-scope variable does not constify it | Fixed here | -- | [gcc-18-lambda-in-predicate-global-not-constified.md](gcc-18-lambda-in-predicate-global-not-constified.md) |
| GCC-19 | A discarded comma operand in a postcondition is wrongly treated as odr-using a by-value parameter | Fixed here | [PR126897](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126897) | [gcc-19-postcondition-comma-odr-use.md](gcc-19-postcondition-comma-odr-use.md) |
| GCC-20 | A pack of non-reference type named in a contract ICEs template substitution | Fixed here | [PR126804](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126804) | [gcc-20-pack-by-value-in-contract.md](gcc-20-pack-by-value-in-contract.md) |
| GCC-21 | `-std=c++26` alone does not link the contract violation handler | Fixed here | [PR126158](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126158) | [gcc-21-cpp26-missing-experimental-link.md](gcc-21-cpp26-missing-experimental-link.md) |
| GCC-22 | A parameter pack of reference type in a contract is rejected outright | Fixed here | [PR126878](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126878) | [gcc-22-pack-reference-constification.md](gcc-22-pack-reference-constification.md) |
| GCC-23 | [dcl.contract.func]/6 not applied to deleted or first-declaration-defaulted functions | Fixed here | [PR124486](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=124486) | [gcc-23-contract-on-deleted-or-defaulted.md](gcc-23-contract-on-deleted-or-defaulted.md) |
| GCC-24 | A noexcept function with a contract segfaults the compiler under `-fno-enforce-eh-specs` | Fixed here | [PR127173](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127173) | [gcc-24-noexcept-body-wrapper-assumption.md](gcc-24-noexcept-body-wrapper-assumption.md) |
| GCC-25 | A postcondition's result-name-introducer does not accept `identifier attribute-specifier-seq :` | Fixed here | [PR125725](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125725) | [gcc-25-postcondition-result-name-attribute.md](gcc-25-postcondition-result-name-attribute.md) |
| GCC-26 | A redeclaration whose parameter type is dependent escapes the postcondition const rule | Fixed here | [PR127196](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127196) | [gcc-26-postcondition-redecl-dependent-param.md](gcc-26-postcondition-redecl-dependent-param.md) |
