# GCC-958: `-ffile-prefix-map` and `-fmacro-prefix-map` do not remap the filenames UBSan and ASan embed

**Status:** Fixed here
**Resolved by:** `0440-sanitizer-prefix-map`
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** none (reproducible-build and
build-path leak; the reports are otherwise correct)
**Upstream Link:** [PR125980](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125980)
(UNCONFIRMED; not ours) reports the ASan half, with a proposed patch for it.
Nothing found for the UBSan half (searched 2026-11-01).
**Affects:** measured 2026-11-01 -- 13.4.0, 16.2.0 and the stock trunk
nightly `17.0.0 20260922` embed the unmapped filename in both; fixed on this
branch.  Clang does not remap them either; it has
`-fsanitize-undefined-strip-path-components=` for UBSan instead.

## Reproducer

Two files.  Each prints `__FILE__` first for comparison, and names itself
`/src/repro.cpp` with a `#line` directive, so the map does not depend on
where the directory is checked out.

[`gcc-958-ubsan-prefix-map.cpp`](gcc-958-ubsan-prefix-map.cpp), at
`-fsanitize=integer-divide-by-zero -ffile-prefix-map=/src=/mapped`, run:

    __FILE__ is /mapped/repro.cpp
    /src/repro.cpp:6:5: runtime error: division by zero

[`gcc-958-asan-prefix-map.cpp`](gcc-958-asan-prefix-map.cpp), at
`-fsanitize=address -ffile-prefix-map=/src=/mapped`, run -- it prints
ASan's description of a global, which a global-buffer-overflow report on
it prints too:

    __FILE__ is /mapped/repro.cpp
    0x... is located 0 bytes after global variable 'XXX' defined in '/src/repro.cpp:1:6' (0x...) of size 10

The unmapped path is in the object file's read-only data whether or not a
report is ever printed, so `strings` on the object shows it too; that is
what trips build-path checks such as OpenEmbedded's `[buildpaths]`.

## Cause

Each site copies the filename into a string constant with no remapping:

* `ubsan_source_location` (`gcc/ubsan.cc`), for every UBSan source
  location, from `expand_location (loc).file`;
* `asan_add_global` (`gcc/asan.cc`), for a global's descriptor -- its
  module name, from `main_input_filename`, and its source location, from
  `expand_location (DECL_SOURCE_LOCATION (decl)).file`;
* `asan_dynamic_init_call` (`gcc/asan.cc`), for the module name passed to
  `__asan_before_dynamic_init`, from `main_input_filename`.

`__FILE__`, `__builtin_FILE ()` and `std::source_location` go through
`remap_macro_filename` (`gcc/file-prefix-map.cc`), which applies
`-fmacro-prefix-map` and `-ffile-prefix-map`.

## Our Fix

All four sites pass the filename through `remap_macro_filename`, so a
sanitizer report names the file as `__FILE__` does.  The two module names
stay identical to each other, which `__asan_before_dynamic_init` relies on
to tell a translation unit's own globals from another's.
`ubsan_use_new_style_p` tests the remapped filename too, because that is
the one libubsan sees: a filename remapped to `""` must still select the
old-style handlers.

Not in LTO, where the prefix maps are deliberately not options (lto-opts.cc
drops them) and a streamed location's filename has already been through
`remap_debug_filename` (lto-streamer-out.cc).  Remapping it again there
applied a map twice whenever lto1 did see it, as with
`-fno-use-linker-plugin`.  So under LTO the sanitizers name the file as
the debug information does, and an ASan global's module name, taken from
the translation unit's name, stays unmapped (it is used only for
initialization-order checking, which is off in LTO).

Tests: `c-c++-common/ubsan/file-prefix-map-1.c`,
`c-c++-common/asan/global-file-prefix-map-1.c` and
`g++.dg/asan/init-order-file-prefix-map.C` (a translation unit's own dynamic
initialization under the map is not an initialization-order fiasco).

## Found by

Surveying which GCC options keep paths and line numbers out of an object
file, and whether they reach contract-violation data; the ASan half had
already been reported upstream.
