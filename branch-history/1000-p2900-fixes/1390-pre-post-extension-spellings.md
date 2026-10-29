---
id: 1390-pre-post-extension-spellings
subject: 'c++: contracts: accept the __pre and __post extension spellings'
depends: []
regenerates: []
fixes: []
---

## Rationale

`contract_assert` has an extension spelling, `__contract_assert`, which is a
keyword in every dialect; `pre` and `post` had none.  Clang accepts `__pre`
and `__post` as well, so GCC does too: being reserved, they can introduce a
function contract specifier wherever contracts are enabled, whatever the
dialect, without the contextual-keyword ambiguity of `pre` and `post`.
`cp_function_contract_specifier_intro` returns the standard name for them,
so everything downstream sees only `pre` and `post`.  With contracts off they
are ordinary identifiers, as before.

## Compile gap

None.

## Contents

- gcc/cp/parser.cc : /The extension spellings __pre and __post, which/
- gcc/doc/invoke.texi : /^As an extension, @code\{__pre\} and @code\{__post\} may be written in place$/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-pre-post-alt-spelling.C : *
