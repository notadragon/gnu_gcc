---
id: 0450-modules-exp-multi-cmi-cleanup
subject: 'testsuite: delete every CMI a grouped module test names'
depends: []
regenerates: []
fixes: [gcc-962]
---

## Rationale

Not a contracts bug, and latent upstream: no upstream grouped test names
more than one CMI.  The grouped-test loop in `modules.exp` appended the
list of CMIs each test names (`module_cmi_p`) to `mod_files` as a single
element, so `cleanup_module_files` tried to delete one file whose name was
the whole list whenever a test named several -- as a test using
`--compile-std-module` names `std`, `std.compat` and `<bits/stdc++.h>`.
Those CMIs then outlived their group, and a later test's `#include` of a
standard header was translated into an import of a `<bits/stdc++.h>` built
with another test's options.  The CMIs are now concatenated, one element
each.

## Compile gap

None.

## Contents

- gcc/testsuite/g++.dg/modules/modules.exp : *
