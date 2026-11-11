# GCC-773: `-fsanitize=vptr` does not check a member of a polymorphic base reached through the wrong derived type

**Status:** Open
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** `wrong-code` (missed check)
**Upstream Link:** None found (searched 2026-10-13: Bugzilla quicksearch
`vptr base`, `fsanitize=vptr`, `vptr member access`; PR116102, PR117976,
PR92678 and PR67515 are other vptr defects, none is this) -- filing owed
**Affects:** measured 2026-10-13 -- 13.4.0, 16.2.0, the stock trunk nightly
`17.0.0 20261005` (`8802cd9fdbe`) and this branch.  Clang 22.1.6 and our
Clang the same.
**Watch test:** `g++.dg/ubsan/open-bug-vptr-base-member.C` (xfail)

## Reproducer

[`gcc-773-vptr-base-member.cpp`](gcc-773-vptr-base-member.cpp), at
`-fsanitize=vptr`, run:

    gcc-773-vptr-base-member.cpp:8:74: runtime error: member access within address 0x... which does not point to an object of type 'B'

That is `own`, a member of `B` read through a `B` glvalue that refers to an
`A`.  `inherited`, which reads `Base::x` through the same `B` glvalue, is
not reported, though [expr.ref] makes it undefined in the same way: the
object expression's type `B` is not similar to the object's type `A`, and
`B` is not a base of `A`.

## Cause

`cp_ubsan_maybe_instrument_member_access` (`gcc/cp/cp-ubsan.cc`) checks the
`COMPONENT_REF` of the member, whose object is the `Base` subobject after
the implicit derived-to-base conversion, so it checks the dynamic type
against `Base`, which an `A` satisfies.  The `B` that the access went
through is not checked.  A fix would also check the object expression's
own class before the conversion to the base.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `sanitizer` |
| Version | `16.2.0` |
| Severity | `normal` |
| Summary | `-fsanitize=vptr does not check a base class member accessed through the wrong derived type` |

````
struct Base { virtual ~Base () {} int x = 1; };
struct A : Base { int a = 2; };
struct B : Base { int b = 3; };
__attribute__((noipa)) int own (A &r) { return reinterpret_cast<B &> (r).b; }
__attribute__((noipa)) int inherited (A &r) { return reinterpret_cast<B &> (r).x; }
int main () { A a; inherited (a); own (a); }

$ g++ -fsanitize=vptr t.cc && ./a.out
t.cc:4:74: runtime error: member access within address 0x... which does not point to an object of type 'B'

Only own () is reported.  inherited () reads Base::x through a B glvalue
that refers to an A, which [expr.ref] makes undefined just the same; the
check is made against the Base subobject, which an A has.  Clang behaves
the same way.
````

## Found by

The M48 work on `ub:expr.ref.member.not.similar` (D4277R1 review F16).
