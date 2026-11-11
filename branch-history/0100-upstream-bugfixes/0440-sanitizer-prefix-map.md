---
id: 0440-sanitizer-prefix-map
subject: 'sanitizer: remap the filenames UBSan and ASan embed with -ffile-prefix-map'
depends: []
regenerates: []
fixes: [gcc-958]
---

## Rationale

GCC-958 (upstream: PR125980 for the ASan half).  Not a contracts bug.
`-ffile-prefix-map` and `-fmacro-prefix-map` remap `__FILE__`,
`__builtin_FILE ()` and `std::source_location`, but the filenames UBSan
and ASan copy into the object -- every UBSan source location, and an ASan
global's descriptor and the module name passed to
`__asan_before_dynamic_init` -- were never remapped, so the build path
leaked into the binary and into the reports.

They now go through `remap_macro_filename`, as `__FILE__` does.  The two
ASan module names are remapped alike, so a translation unit's own globals
still match the module it initializes.  `ubsan_use_new_style_p` tests the
remapped filename, the one libubsan sees.

Under LTO nothing is remapped again: the prefix maps are not LTO options,
and a streamed location's filename has already been through
`remap_debug_filename`.  So under LTO an ASan global's filename follows
`-ffile-prefix-map` and `-fdebug-prefix-map` but not `-fmacro-prefix-map`
alone, which the `-fsanitize=address` documentation now says; the
`-fmacro-prefix-map` tests skip LTO, and `global-macro-prefix-map-lto.C`
and `global-debug-prefix-map-lto.C` pin the LTO behaviour.

## Compile gap

None.  Everything named is upstream's; the tests use no contracts.

## Contents

- gcc/ubsan.cc : /file-prefix-map\.h/, /strlen \(xloc\.file\)/, /The filename libubsan sees is the remapped one/, /startswith \(xloc\.file/
- gcc/asan.cc : /file-prefix-map\.h/, /remap_macro_filename/
- gcc/doc/invoke.texi : /^The name of the file that defines a global variable, which$/
- gcc/testsuite/c-c++-common/ubsan/file-prefix-map-1.c : *
- gcc/testsuite/c-c++-common/asan/global-file-prefix-map-1.c : *
- gcc/testsuite/g++.dg/asan/init-order-file-prefix-map.C : *
- gcc/testsuite/c-c++-common/ubsan/macro-prefix-map-1.c : *
- gcc/testsuite/c-c++-common/asan/global-macro-prefix-map-1.c : *
- gcc/testsuite/g++.dg/asan/global-macro-prefix-map-lto.C : *
- gcc/testsuite/g++.dg/asan/global-debug-prefix-map-lto.C : *
