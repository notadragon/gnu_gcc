---
id: 0130-explicit-inst-pack-cv
subject: 'c++: record the cv-qualified pack explicit-instantiation defect'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not a contracts bug, and NOT FIXED here -- this commit only records it.
Explicit instantiation of a variadic function template fails to match when a
template argument bound to the function parameter pack is cv-qualified.  The
instantiations are well-formed and Clang accepts them; GCC rejects them with
"template-id does not match any template declaration".  The test is marked
`dg-bogus` xfail so it XPASSes once the defect is fixed upstream.

## Compile gap

None.  A pure test addition against upstream behaviour, with no dependency on
anything this branch adds.

## Contents

- gcc/testsuite/g++.dg/template/explicit-inst-pack-cv.C : *
