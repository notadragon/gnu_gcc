# GCC-786: Binding a reference through a pointer to an automatic object whose scope has ended accepted in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-13) -- filing owed; related PR70331
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`), 16.2.0, 15.3.0, 14.4.0; this branch's install of 2026-10-13
**D4277R1 category:** `ub:expr.unary.dereference (.indirection), basic.compound.invalid.pointer (.indirection)`

## Reproducer

[`gcc-786-constexpr-reference-through-dangling-pointer.cpp`](gcc-786-constexpr-reference-through-dangling-pointer.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool t() {
      int* p;
      { int y = 1; p = &y; }
      int& r = *p;   // UB [expr.unary.op]/1: p points to no object
      (void)r;
      return true;
    }
    static_assert(t());

Accepted (the core undefined behavior is not diagnosed, so the expression
is wrongly treated as a constant expression).  Reading through `p` is
rejected ("accessing 'y' outside its lifetime", the lifetime tracking of
PR70331 / PR96630).  The past-the-end form is GCC-763.  Clang accepts it
too (llvm_llvm-project CLANG-717).

## Upstream search (2026-10-13)

Bugzilla REST, component c++, control validated (the summary phrase
"Calling member function before base subobject" returned PR126357).
Queries: `s "dangling constexpr"` (PR70331 and PR96630, both reads, fixed
by r14-2773; PR110619, a dangling pointer converting to nullptr),
`s "dangling constant"`, `s "dangling reference constant expression"`,
`s "out of scope constexpr"` (also PR118461, re-entering a scope),
`s "lifetime reference constexpr"`.  None is a reference binding.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Binding a reference through a dangling pointer accepted in constant evaluation
**Description:** The program below has core-language undefined behavior ([expr.unary.op]; D4277R1 category `ub:expr.unary.dereference`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  Reading through the pointer is rejected (PR70331).

    constexpr bool t() {
      int* p;
      { int y = 1; p = &y; }
      int& r = *p;
      (void)r;
      return true;
    }
    static_assert(t());

Found by the CLANG-717 mirror (M52-GCC, 2026-10-13).
