# GCC-877: `-fsanitize=alignment` does not check a placement new that stores nothing

**Status:** Open (deferred here under DECISIONS.md E126 (a); filing owed)
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** `missed-diagnostic`
**Upstream Link:** None found (searched 2026-10-18: Bugzilla quicksearch
`placement new alignment sanitize`, `fsanitize=alignment placement`,
`sanitize alignment placement new`, `reference binding misaligned`; the
`sanitizer` component's summaries containing `placement` or `align`;
PR66308, the missing downcast check, is another gap) -- filing owed
**Affects:** measured 2026-10-18 -- 13.4.0, 14.4.0, 15.3.0, 16.2.0, the
Compiler Explorer trunk build `17.0.0 20261005` (`8802cd9fdbe`), the stock
trunk build `17.0.0 20260922` (`gnu_gcc-trunk-install`) and this branch
(the install of the `f313ea58899` tree), -O0 and -O2.  Clang's
`-fsanitize=alignment` checks the pointer ("constructor call on misaligned
address"), except when it is arithmetic on a character array
(llvm_llvm-project M94-Clang (1)).
**Watch test:** `g++.dg/ubsan/open-bug-placement-new-align-nostore.C` (xfail)

## Reproducer

[`gcc-877-placement-new-align-nostore.cpp`](gcc-877-placement-new-align-nostore.cpp),
at `-fsanitize=alignment`, run:

    empty
    dflt
    store
    gcc-877-placement-new-align-nostore.cpp:9:93: runtime error: store to misaligned address 0x000000404091 for type 'int', which requires 4 byte alignment
    done

Creating an object at a misaligned address is undefined
([basic.align], [expr.new]): `empty`, an empty class value-initialized,
and `dflt`, a class default-initialized (no initialization performed),
store nothing and are not reported; `store`, the control, is reported by
the check of its store.

## Cause

GCC's `-fsanitize=alignment` checks the loads and stores of the
initialization (`instrument_mem_ref`, `gcc/ubsan.cc`) and reference
bindings, never the pointer a placement new is given (`build_new_1`,
`gcc/cp/init.cc`, makes no check).  A creation that stores nothing is
therefore never checked.

## On this branch

Under `-fcontracts-p3100` the native check
`ub:basic.align.object.alignment.nostore` checks that pointer when the
new-expression stores nothing (`build_implicit_placement_new_align_check`,
GCC-738): both forms are reported
(`g++.dg/contracts/cpp26/p3100-placement-align-char-array*.C`).  The
sanitizer's own behaviour, this report's subject, stays stock.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `sanitizer` |
| Version | `16.2.0` |
| Severity | `enhancement` |
| Summary | `-fsanitize=alignment does not check the pointer of a placement new` |

````
#include <new>
struct alignas (4) S {};
struct T { int a, b; };
__attribute__((noipa)) void empty (char *b) { new (b) S (); }
__attribute__((noipa)) void dflt (char *b) { new (b) T; }
alignas (8) char buf[16];
int main () { empty (buf + 1); dflt (buf + 1); }

$ g++ -fsanitize=alignment t.cc && ./a.out
(nothing)

Both create an object at a misaligned address, which is undefined, but
neither stores anything, and -fsanitize=alignment checks only the stores
of the initialization.  Clang checks the placement new's pointer:
"constructor call on misaligned address 0x... for type 'S', which
requires 4 byte alignment".
````

## Found by

M93-GCC, writing the GCC mirror of the stock Clang miss of a placement
new on character-array arithmetic (llvm_llvm-project M94-Clang (1),
found by the M-Docs (part 2) audit, slice A); D4277R1's
`basic.align.object.alignment` details already say GCC's sanitizer misses
`.nostore`.
