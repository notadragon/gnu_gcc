# GCC-854: `-fsanitize=null` does not check a member call that returns a class by value

**Status:** Open (deferred here under DECISIONS.md E113 (b); filing owed)
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** `wrong-code` (missed check)
**Upstream Link:** None found (searched 2026-10-16: Bugzilla quicksearch
`sanitize=null member call class return`, `fsanitize=null AGGR_INIT_EXPR`,
`ubsan null member call`, `sanitize null this`, `UBSAN_NULL member call`,
`fsanitize=null not detected`; PR63426 is the -fsanitize=undefined
meta-bug, PR67515 and PR79265 other defects, none is this) -- filing owed
**Affects:** measured 2026-10-16 -- 13.4.0, 16.2.0, the stock trunk build
`17.0.0 20260922` (`gnu_gcc-trunk-install`) and this branch (the install of
the `6ff96632429` tree), -O0.  Clang not measured (llvm_llvm-project task
row from M84-GCC).
**Watch test:** `g++.dg/ubsan/open-bug-null-member-call-class-return.C`
(xfail)

## Reproducer

[`gcc-854-null-member-call-class-return.cpp`](gcc-854-null-member-call-class-return.cpp),
at `-fsanitize=null`, run:

    cls done
    gcc-854-null-member-call-class-return.cpp:9:52: runtime error: member call on null pointer of type 'struct S'

`scalar`, a member function returning `int` called through a null pointer,
is reported.  `cls`, the same call of a member function returning `R`, a
class with a non-trivial copy constructor and destructor, is not.

On this branch the P3100 null check (`ub:expr.unary.dereference.nullptr`)
missed it the same way until M88-GCC (2026-10-17), which fixed the P3100
half under DECISIONS.md E113 (b) (`cp_ubsan_maybe_check_aggr_init_call`);
the stock `-fsanitize=null` miss is deferred there, this report's subject.

## Cause

`cp_genericize_r` (`gcc/cp/cp-gimplify.cc`) instruments the object pointer
of a member call (`ubsan_maybe_instrument_member_call`) in its `CALL_EXPR`
case only.  A call that returns a class needing a temporary of its own is
an `AGGR_INIT_EXPR` until gimplification, and that case falls through
below the instrumentation, so its `this` argument is never checked.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `sanitizer` |
| Version | `16.2.0` |
| Severity | `normal` |
| Summary | `-fsanitize=null does not check a member call returning a class by value` |

````
struct R { R () {} R (const R &) {} ~R () {} int r; };
struct S { R f () { return R (); } int g () { return 0; } };
__attribute__((noipa)) void cls (S *sp) { sp->f (); }
__attribute__((noipa)) void scalar (S *sp) { sp->g (); }
int main () { S *volatile sp = 0; cls (sp); scalar (sp); }

$ g++ -fsanitize=null t.cc && ./a.out
t.cc:4:52: runtime error: member call on null pointer of type 'struct S'

Only scalar () is reported.  The call in cls () is an AGGR_INIT_EXPR,
which cp_genericize_r does not instrument (only CALL_EXPR).
````

## Found by

M84-GCC, measuring DECISIONS.md E110 (a) on calls whose result is a class.
