# GCC-36: a non-template's postcondition const diagnostic is emitted twice

**Status:** Open on stock upstream; **not present on this branch**
**Resolved by:** `1060-postcondition-odr-use` -- incidentally, not
deliberately: that commit reworks `check_postcondition_odr_use_r`, and the
divergence that keeps the diagnostic single-shot here is a side effect of
that rework rather than something aimed at this defect
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `diagnostic`
**Upstream Link:** [PR127297](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127297)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-06 before filing:
nothing matched.)
**Affects:** measured 2026-09-09 -- doubled on 16.1.0, 16.2.0 and both trunk
builds. 15.3.0 predates contracts. This branch reports it once, having
diverged in `check_postcondition_odr_use_r`, so the defect is upstream's
alone; it is recorded here because the entry criterion for this directory is
reproduction on stock.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `minor` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] the postcondition const-parameter diagnostic is emitted twice for a non-template function` |

Attachments:

| File | Description |
|---|---|
| [`postcondition-const-nontemplate.C`](postcondition-const-nontemplate.C) | One line; the const-parameter diagnostic is emitted twice for it |

````
For a non-template function, the [dcl.contract.func] const-parameter
diagnostic is emitted twice: byte-identical message, byte-identical
location, and an identical note each time.

```
int f(int v) post(r : v == r) { return v; }
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -fsyntax-only \
    postcondition-const-nontemplate.C
postcondition-const-nontemplate.C:7:23: error: a value parameter used in a postcondition must be const
    7 | int f(int v) post(r : v == r) { return v; }
      |                       ^
postcondition-const-nontemplate.C:7:11: note: parameter declared here
    7 | int f(int v) post(r : v == r) { return v; }
      |       ~~~~^
postcondition-const-nontemplate.C:7:23: error: a value parameter used in a postcondition must be const
    7 | int f(int v) post(r : v == r) { return v; }
      |                       ^
postcondition-const-nontemplate.C:7:11: note: parameter declared here
    7 | int f(int v) post(r : v == r) { return v; }
      |       ~~~~^
```

The diagnosis itself is correct -- the program is ill-formed and should be
rejected.  Only the duplication is the defect.

Worth distinguishing from a case that is not a defect: for a function
template, two different diagnostics are emitted, one naming the parameter
and one pointing at the contract.  Those come from two mechanisms that
deliberately anchor at different places, and with more than one parameter or
more than one postcondition they say genuinely different things.  It is only
the non-template path above, where the same mechanism fires twice with the
same words at the same column, that is wrong.

DISCOVERY

Found while fixing a neighbouring defect in the same rule, by reading the
diagnostics the fix produced rather than only whether the program was
rejected -- the duplication is invisible to a test that just checks for an
error.

ANALYSIS

Both emissions come from the same place with the same location, so this is
the odr-use walk reaching the parameter twice rather than the two distinct
mechanisms firing.  A postcondition's predicate is examined once through the
declaration and once more when the definition is processed, and for a
non-template both examinations reach the same tree.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       emitted twice
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes
  local build -g      17.0.0 20260909, 7dab38c9d71  yes

```
$ ./gcc-16.2.0/bin/g++ -v
Using built-in specs.
COLLECT_GCC=./gcc-16.2.0/bin/g++
COLLECT_LTO_WRAPPER=.../gcc-16.2.0/bin/../libexec/gcc/x86_64-linux-gnu/16.2.0/lto-wrapper
Target: x86_64-linux-gnu
Configured with: ../gcc-16.2.0/configure --prefix=/opt/compiler-explorer/gcc-build/staging --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --disable-bootstrap --enable-multiarch --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --enable-clocale=gnu --enable-languages=c,c++,fortran,ada,objc,obj-c++,go,d,m2,rust,cobol,algol68 --enable-ld=yes --enable-gold=yes --enable-libstdcxx-time=yes --enable-linker-build-id --enable-lto --enable-plugins --enable-threads=posix --with-pkgversion=Compiler-Explorer-Build-gcc--binutils-2.44
Thread model: posix
Supported LTO compression algorithms: zlib
gcc version 16.2.0 (Compiler-Explorer-Build-gcc--binutils-2.44)
```
````

## Reproducer

[`postcondition-const-nontemplate.C`](postcondition-const-nontemplate.C).

## What this issue is not: the two-diagnostic template case

It was originally filed as "the rule is reported twice for a function
template", on the strength of this:

```c++
template <class T> T f (T v) post (r: v == r) { return v; }
int use () { return f (1); }
```

```
:1:27: error: value parameter 'v' used in a postcondition must be const
:1:39: error: a value parameter used in a postcondition must be const
```

**That is not a defect.** Two mechanisms implement the rule and they anchor at
different places on purpose:

* `check_postcondition_parm_in_redecl` reports **at the parameter**, naming it
  -- "which parameter is wrong";
* `check_postcondition_odr_use_r` reports **at the contract** -- "which
  postcondition uses it".

With one parameter and one postcondition those collapse onto adjacent columns
and look redundant, which is what the minimal reproducer above suggests. With
several they are complementary, and the testsuite already depends on it:
`dcl.contract.func.p7-t1.C` has a five-parameter function with four
postconditions and expects **both** families -- three "value parameter 'i'/'k'/'l'"
at the parameter list, and three "a value parameter ..." at the individual
offending `post` clauses. Our branch and stock trunk both emit 28 errors for
that file; they agree exactly.

A fix was attempted and reverted. Suppressing the walk when the carry-over had
already reported -- derivable without new state, since the carry-over's
condition is the walk's plus `!TREE_READONLY` -- works on the minimal
reproducer and destroys the per-contract diagnostics, taking 65 test results
with it. The lesson is that the minimal reproducer was not representative:
**check a multi-contract, multi-parameter function before concluding that two
diagnostics for one rule are redundant.**

An earlier attempt failed differently and is worth not repeating either:
skipping on `parm_used_in_post_p` alone conflates "the carry-over ran" with
"the carry-over reported", and contract constification sets `TREE_READONLY`, so
for a constified parameter the carry-over marks without diagnosing and the walk
is the only reporter.

## Impact

Diagnostic quality only, on stock, for non-template functions. Both messages
are true and the program is correctly rejected.

## Bugzilla

Searched 2026-09-06 by summary substring `postcondition`: 18 PRs, none about a
duplicated diagnostic. Nearest neighbours are different symptoms in the same
code: **PR124395**, **PR127196**, **PR126897**. Quicksearch on the diagnostic
text returns nothing.
