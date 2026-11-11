# GCC-831: an empty class stored to a past-the-end element accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-15) -- filing owed; related PR108537
**Affects:** stock trunk `17.0.0 20260922` (`8802cd9fdbe`), 16.2.0, 15.3.0, 14.4.0; this branch's install of 2026-10-15
**D4277R1 category:** `ub:expr.unary.dereference (.invalid.known)`

## Reproducer

[`gcc-831-constexpr-empty-store-past-end.cpp`](gcc-831-constexpr-empty-store-past-end.cpp), at `-std=c++26 -fsyntax-only`:

    struct E {};
    constexpr bool f() { E a[1]; E e; a[1] = e; return true; }   // UB [expr.unary.op]/1
    static_assert(f());

Accepted.  `a[1]` is `*(a + 1)`, an indirection through a pointer that
points to no object ([expr.unary.op]/1), and the trivial assignment of an
empty class then writes nothing: GCC elides it and never evaluates the
target.  The copy from it, `E e2 = a[1]`, is rejected ("array subscript
value '1' is outside the bounds").  Clang rejects the store ("member call
on dereferenced one-past-the-end pointer").  Found while pinning the
constant evaluation of DECISIONS.md E95 (a) (M74-GCC); the run-time check
GCC-827 added is not evaluated in constant evaluation.

## The same miss: assigned from it (added 2026-10-16, M76-GCC)

[`gcc-831-constexpr-empty-assign-from-past-end.cpp`](gcc-831-constexpr-empty-assign-from-past-end.cpp),
at `-std=c++26 -fsyntax-only`:

    struct E {};
    constexpr bool f() { E a[1]; E e; e = a[1]; return true; }   // UB [expr.unary.op]/1
    static_assert(f());

Accepted by stock trunk (`8802cd9fdbe`), 16.2.0, 15.3.0 and 14.4.0, and by
this branch.  The elided trivial assignment of an empty class evaluates
both operands as glvalues only, and the evaluator does not diagnose a
glvalue designating one past the end unless it is read or written; so the
source of the assignment is no more checked than its target.  An empty
base of a non-empty element is the same: `static_cast<EB &> (da[1]) = b`
and `b = da[1]` (`struct D : EB { int x; }; D da[1];`) are accepted.  The
copy constructor, `E e2 = a[1]` and `EB b = da[1]`, keeps its source and
is rejected.  Clang (trunk) rejects the two base forms and accepts `e =
a[1]` (llvm_llvm-project CLANG-779).  Filed by M76-GCC from llvm_llvm-project
CLANG-779's watch test (the `from` form); folded into this row as the same
miss rather than given its own ID.  The watch test
`open-bug-constexpr-empty-store-past-end.C` carries the three forms,
xfailed.  Upstream search 2026-10-16 (Bugzilla REST quicksearch:
`empty class assignment past the end constexpr`, `one-past-the-end
constexpr assignment`, `trivial assignment empty constexpr`, `one past the
end constant expression`, `constexpr past end`; control: `108537` returned
PR108537): nothing new.  PR108537 (a discarded dereference past the end
accepted) is the general form.

## Upstream search (2026-10-15)

Bugzilla REST quicksearch, component c++, control validated (the phrase
"one-past-the-end constexpr" returned PR90666 and others, none this bug):
`empty class past the end constexpr` (nothing), `one-past-the-end
constexpr`, `past-the-end constant`.  PR108537 (a discarded `b[100]` past
the end accepted) is related.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Assignment of an empty class to a past-the-end array element accepted in constant evaluation
**Description:** The program below has core-language undefined behavior ([expr.unary.op]; D4277R1 category `ub:expr.unary.dereference`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted (the elided empty-class assignment never evaluates its target).  Copying from the same element is rejected.  Related PR108537.

    struct E {};
    constexpr bool f() { E a[1]; E e; a[1] = e; return true; }
    static_assert(f());
