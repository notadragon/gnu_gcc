---
id: 4290-identification-label-range-watch-test
subject: 'c++, contracts: test the range forms of an identification label'
depends: [4200-p3400-core, 4240-p3400-facet-identification]
regenerates: []
fixes: []
---

## Rationale

Test the forms of `group_names` that P3400 [support.contract.control.ident]
allows: any const range whose elements convert to `string_view`.  Each
label names the `safety` group last and that group is ignored, so a
violated precondition is evaluated during constant evaluation only when
the label's groups are not read in full.  An array of `const char *` is the
control; a `std::array` and a C array of `string_view`, an
`initializer_list` and a `span` over three names are the range forms.

The library concept requires `group_names[0]` convertible to `const char *`
and both the library and the compiler count names by `sizeof`, so the
`string_view` and `initializer_list` forms are rejected and their groups
ignored, and only two of the `span`'s names are read; those assertions are
expected to fail (GCC-899 in `open-issues/README.md`).

## Compile gap

None.  A pure test addition.  It uses the group-semantic option and the
identification facet, so it follows both.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-identification-label-range.C : *
