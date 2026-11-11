# GCC-962: `modules.exp` leaves behind the CMIs of a grouped test that names several

**Status:** Fixed here
**Component:** testsuite
**Keywords (ours -- upstream sets its own):** none (testsuite harness)
**Upstream Link:** None found (searched 2026-11-03: Bugzilla quicksearch
`modules.exp`, `cleanup_module_files`, `mod_files`, `module_cmi_p`,
`compile-std-module testsuite`, `modules gcm.cache stale`, `grouped module
test CMI`, `stale CMI testsuite`; PR100052, PR103324, PR111300, PR124200,
PR124641 and PR124888 are other modules or testsuite issues, none is this)
-- filing owed
**Affects:** `gcc/testsuite/g++.dg/modules/modules.exp` on upstream master
(`7ced41f4d43`, 2026-11-04, line 421) and every release that has the
grouped-test loop; latent upstream, where no grouped test names more than
one CMI in one file (`compile-std1.C`, the one upstream test naming three,
is not grouped).  Found by M1-GCC of the D4451 program, whose grouped
`--compile-std-module` tests left a `<bits/stdc++.h>` CMI behind.
**Resolved by:** [49c78c988455](https://github.com/notadragon/gnu_gcc/commit/49c78c98845568ca5854e72868792e0ae597c342)

## Symptom

A grouped module test (`foo_a.C`, `foo_b.C`, ...) one of whose files names
more than one CMI with `dg-module-cmi` -- as every `--compile-std-module`
test does, naming `std`, `std.compat` and `<bits/stdc++.h>` -- passes, and
none of the CMIs that file names is deleted at the end of the group.  They
stay in `gcm.cache/` for the rest of the run.  On this branch that turned
into wrong results: a later test's `#include` of a standard header was
translated into an import of the leftover `<bits/stdc++.h>` CMI, built with
another test's options (a different contracts option set), so the later
test saw declarations its own options do not provide.

Reproducer: [`gcc-962-multi-cmi-leak_a.C`](gcc-962-multi-cmi-leak_a.C) and
[`gcc-962-multi-cmi-leak_b.C`](gcc-962-multi-cmi-leak_b.C), run as a group.
Measured 2026-11-03 in this branch's release build tree with upstream's
`modules.exp` (the file as it was before
[49c78c988455](https://github.com/notadragon/gnu_gcc/commit/49c78c98845568ca5854e72868792e0ae597c342)): all 18 results pass (three
dialects), and `gcc/testsuite/g++/gcm.cache/` is left holding `std.gcm`,
`std.compat.gcm`, `leak.gcm` and the `<bits/stdc++.h>` header unit's CMI.
With this branch's `modules.exp` the same run leaves none of them.  A
single-file test naming the same CMIs (`compile-std1.C`) cleans up.

## Cause

In the grouped-test loop of `modules.exp`,

    lappend mod_files [module_cmi_p $testcase $module_cmis $std]

appends the list `module_cmi_p` returns -- every CMI that test named -- to
`mod_files` as one element.  `cleanup_module_files` then calls
`file_on_host delete` once per element, and DejaGnu's `standard_file
delete` deletes an argument only if `[file isfile $x]`: an element that is
a list of two or more paths names no file, so nothing is deleted, silently.
A test naming one CMI produces a one-element list, which is the same string
as the path, so the defect never shows for those.  The ungrouped loop
passes `module_cmi_p`'s result straight to `cleanup_module_files` and is
correct.

The same in a few lines of Tcl:

    set mod_files {}
    lappend mod_files [list gcm.cache/x.gcm]
    lappend mod_files [list gcm.cache/std.gcm gcm.cache/std.compat.gcm]
    foreach file $mod_files { if {[file isfile $file]} { file delete $file } }
    # leaves gcm.cache/std.gcm and gcm.cache/std.compat.gcm

## Fix (this branch)

Concatenate instead, one element per CMI:

    set mod_files [concat $mod_files \
                       [module_cmi_p $testcase $module_cmis $std]]

No `verify-cases.txt` row: the defect is in the DejaGnu harness, which
`verify.sh` cannot measure.  At a rebase, `git show
upstream/master:gcc/testsuite/g++.dg/modules/modules.exp | grep 'lappend
mod_files'` says whether upstream still has it.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `testsuite` |
| Version | `16.2.0` |
| Severity | `normal` |
| Summary | `modules.exp: grouped test naming several CMIs leaves them all behind` |

````
In the grouped-test loop of gcc/testsuite/g++.dg/modules/modules.exp,

    lappend mod_files [module_cmi_p $testcase $module_cmis $std]

adds the list of CMIs a test names to mod_files as a single element.
cleanup_module_files then does "file_on_host delete" once per element,
and DejaGnu's standard_file only deletes an argument that is a file, so
when a test names two or more CMIs (dg-module-cmi), the element is a
list of paths and none of them is deleted.  Single-CMI tests are
unaffected because a one-element list is the path itself; the ungrouped
loop passes module_cmi_p's result directly and is fine.

No upstream grouped test names more than one CMI yet, so nothing fails
today, but a grouped --compile-std-module test (naming std, std.compat
and <bits/stdc++.h>) leaves those CMIs in gcm.cache for the rest of the
run, where a later test's #include of a standard header can be
translated into an import of a <bits/stdc++.h> built with the earlier
test's options.

Reproducer: g++.dg/modules/leak_a.C

  // { dg-additional-options "-fmodules --compile-std-module" }
  // { dg-do compile { target c++20 } }
  // { dg-module-cmi std }
  // { dg-module-cmi std.compat }
  // { dg-module-cmi <bits/stdc++.h> }
  // { dg-module-cmi leak }
  export module leak;
  import std;
  export inline std::size_t n () { return 0; }

and g++.dg/modules/leak_b.C

  // { dg-additional-options "-fmodules" }
  // { dg-do compile { target c++20 } }
  import leak;
  int i = n ();

make check-g++ RUNTESTFLAGS="modules.exp=leak*" passes and leaves
std.gcm, std.compat.gcm, leak.gcm and the <bits/stdc++.h> CMI in
gcc/testsuite/g++/gcm.cache.  Adding each CMI to mod_files as its own
element (concatenating the lists) makes the cleanup delete them.
````
