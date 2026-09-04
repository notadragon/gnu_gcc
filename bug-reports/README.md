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
