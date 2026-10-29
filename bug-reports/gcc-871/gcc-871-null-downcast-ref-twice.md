# GCC-871: `-fsanitize=null` reports a null downcast to a reference twice

**Status:** Open (deferred here under DECISIONS.md E124 (a); filing owed)
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** `wrong-code` (duplicate
diagnostic)
**Upstream Link:** None found (searched 2026-10-17: Bugzilla quicksearch
`reference binding null downcast`, `sanitize=null reference binding twice`,
`fsanitize=null static_cast reference`, `reference binding to null
pointer`, `ubsan reference binding`, `UBSAN_NULL reported twice`,
`sanitize=null duplicate`, `ubsan downcast null`; PR98206, a misaligned
reference downcast fixed in GCC 11, and PR100415 are other defects) --
filing owed
**Affects:** measured 2026-10-17 -- 13.4.0, 14.4.0, 15.3.0, 16.2.0, the
Compiler Explorer trunk build `17.0.0 20261005` (`8802cd9fdbe`), the stock
trunk build `17.0.0 20260922` (`gnu_gcc-trunk-install`) and this branch
(the install of the `7ed1969fc68` tree), -O0 and -O2.  Stock Clang does
not report the downcast at all (llvm_llvm-project CLANG-803).
**Watch test:** `g++.dg/ubsan/open-bug-null-downcast-ref-twice.C` (xfail)

## Reproducer

[`gcc-871-null-downcast-ref-twice.cpp`](gcc-871-null-downcast-ref-twice.cpp),
at `-fsanitize=null`, run:

    bind
    gcc-871-null-downcast-ref-twice.cpp:7:70: runtime error: reference binding to null pointer of type 'struct D2'
    gcc-871-null-downcast-ref-twice.cpp:7:70: runtime error: reference binding to null pointer of type 'struct D2'
    bind0
    gcc-871-null-downcast-ref-twice.cpp:6:71: runtime error: reference binding to null pointer of type 'struct D0'
    done

`bind`, the base at offset 16 in `D2`, is reported twice; `bind0`, the
base at offset zero in `D0`, once.  (`bind` comes first so that
`verify.sh`'s 160-column digest tells the two apart.)  So is the binding in a read through
the downcast or the address of its member (three reports with the
member access), and a virtual call through it reports the binding and the
vtable pointer's member access twice each (the object expression is
evaluated twice).

## Cause

With the base at a nonzero offset, `build_base_path` wraps the downcast in
a null test (`-fsanitize=null` clears `flag_delete_null_pointer_checks`):
`p != 0 ? (D2 &) (p - 16) : (D2 &) 0`.  `build_static_cast_1`
(`gcc/cp/typeck.cc`) instruments the null arm's reference binding
directly ("make sure to diagnose reference binding to NULL even when the
reference is converted to pointer later on"), giving `(.UBSAN_NULL (0),
(D2 &) 0)`; `cp_genericize_r` (`gcc/cp/cp-gimplify.cc`) then instruments
the `INTEGER_CST` of reference type left at the end of that comma again.
At offset zero there is no null test and one instrumentation.

## On this branch

Under `-fcontracts-p3100`, where a leaf of
`ub:expr.unary.dereference.nullptr` is checked, the conversion makes no
null test and the null check of the downcast's use, the P3100 check and
`-fsanitize=null`'s together, tests `p` itself: once (GCC-869, GCC-870,
fixed in M90-GCC; `g++.dg/ubsan/p3100-null-downcast-ref-offset-sanitize-null*.C`).
With the group ignored or assumed, and without `-fcontracts-p3100`, the
stock behaviour stays, this report's subject.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `sanitizer` |
| Version | `16.2.0` |
| Severity | `normal` |
| Summary | `-fsanitize=null reports a null static_cast to a reference twice` |

````
struct V { virtual void h () {} int v; };
struct X { virtual void x () {} long pad; };
struct D2 : X, V { int d; };
__attribute__((noipa)) D2 &bind (V *p) { return static_cast<D2 &> (*p); }
int main () { V *volatile p = 0; bind (p); }

$ g++ -fsanitize=null t.cc && ./a.out
t.cc:4:70: runtime error: reference binding to null pointer of type 'struct D2'
t.cc:4:70: runtime error: reference binding to null pointer of type 'struct D2'

build_static_cast_1 instruments the null arm of the downcast's null test,
(D2 &) 0, and cp_genericize_r instruments the INTEGER_CST it leaves again.
With V at offset zero (struct D2 : V) it is reported once.
````

## Found by

M90-GCC, mirroring llvm_llvm-project's CLANG-803 watch test (GCC-868).
