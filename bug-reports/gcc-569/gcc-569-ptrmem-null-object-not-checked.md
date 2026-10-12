# GCC-569: A null object pointer through `->*` (pointer to data member) is not checked

**Status:** Fixed here (commit [95dd23d3644a](https://github.com/notadragon/gnu_gcc/commit/95dd23d3644a42248b5128c41586c6a162f0c971))
**Resolved by:** `0190-ubsan-null-ptrmem-object`
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** `wrong-code` (missed check)
**Upstream Link:** None found (searched 2026-09-26: Bugzilla REST, open bugs
whose summary has `pointer to member`, `pointer-to-member`, `member
pointer`, `ptrmem` or `->*` together with a sanitizer, plus comment text on
`fsanitize=null member pointer`, `ubsan pointer-to-member` and `ptrmem
sanitize`; PR100313, PR117976 and PR92678 are about pointers to member
functions, none is this.  Earlier search 2026-09-25, same result)
**Affects:** measured 2026-09-26 -- 16.1.0, 16.2.0 and stock trunk
`17.0.0 20260922` (`f008f03eff8`) fault with no report at -O0 and -O2;
fixed on this branch, for `-fsanitize=null` and the P3100 check alike.
**Clang:** stock `-fsanitize=null` misses it at -O0 too (at -O2 it reports a
null load); the Clang branch's P3100 check the same (CLANG-565 in that fork)

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `sanitizer` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `-fsanitize=null does not check the object pointer of ->* with a pointer to data member` |

Attachments:

| File | Description |
|---|---|
| [`ptrmem-null-object-not-checked.cpp`](ptrmem-null-object-not-checked.cpp) | `np->*pm` with a null `np` |

````
[expr.mptr.oper] defines E1->*E2 as (*(E1)).*E2, so a null E1 is a null
pointer dereference, but -fsanitize=null does not check it.

```
struct S { int a; int b; };
S *volatile np = nullptr;
int S::*volatile pm = &S::b;
__attribute__((noinline)) int ptm () { return np->*pm; }
int main () { return ptm (); }
```

```
$ g++ -fsanitize=null ptrmem-null-object-not-checked.cpp && ./a.out
Segmentation fault (core dumped)
```

Expected: a report before the fault, as for np->b, which gives

  runtime error: member access within null pointer of type 'struct S'

The same happens at -O2, and with (*np).*pm spelled out.


DISCOVERY

Found while testing null-pointer checks across the ways C++ can
dereference a pointer.


ANALYSIS

From reading the code, not confirmed in a debugger: the access is lowered
by build_m_component_ref (gcc/cp/typeck2.cc) to a memory reference whose
base is np + pm, with the offset already folded in.  instrument_null /
instrument_mem_ref (gcc/ubsan.cc) check that base, which is not null, so the
object pointer itself is never tested.  Emitting the null check on the
object pointer in build_m_component_ref, as is done for member calls, would
catch it.


VERSIONS -- all on x86_64-linux-gnu

  source              version                       -O0 report  -O2 report
  compiler-explorer   16.1.0                        no          no
  compiler-explorer   16.2.0                        no          no
  compiler-explorer   17.0.0 20260922, f008f03eff8  no          no
````

## Our Fix

`gcc/cp/typeck2.cc`, `build_m_component_ref`: the object pointer is checked
while it is still an operand, before the member's offset is added, through
a new `ubsan_maybe_instrument_ptrmem_object` (`gcc/c-family/c-ubsan.cc`) that
reuses the member-call path with `UBSAN_MEMBER_ACCESS`.  This fixes stock
`-fsanitize=null` and the branch's P3100 null check alike.  Regression tests:
`g++.dg/ubsan/null-ptrmem-object.C` and
`g++.dg/contracts/cpp26/p3100-null-ptrmem.C`.

## Notes (ours, not for the report)

Found by review round #12 (fan-out g-e-3) through the branch's P3100 null
check (same lowering, config `ub:expr.unary.dereference.nullptr` at
`noexcept_enforce`), which faulted with no handler call while every other
null spelling the fan-out tried was caught.  The Clang fork has the same
miss at -O0 (CLANG-565 there).
