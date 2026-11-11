# GCC-755: Referring to a member in a constructor's function-try-block handler is not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`); this branch's install of 2026-10-12
**D4277R1 category:** `ub:except.handle.handler.ctor.dtor (.initialized)`

## Reproducer

[`gcc-755-constexpr-function-try-block-handler-member.cpp`](gcc-755-constexpr-function-try-block-handler-member.cpp), at `-std=c++26 -fsyntax-only`:

    struct S {
      int x;
      constexpr S(bool b) try : x(1) { if (b) throw 0; }
      catch (...) { int y = x; (void)y; }   // UB [except.handle]/10
    };
    constexpr bool f() { try { S s(true); } catch (...) {} return true; }
    static_assert(f());

Accepted (the core undefined behavior is not diagnosed, so the expression is wrongly treated as a constant expression).  A never-initialized or already-destroyed member or base is rejected; an initialized scalar member, and `&y`, are accepted.

## Upstream search (2026-10-12)

Bugzilla REST, controls validated (a phrase from PR126425, PR126357 and PR126204 each returned its PR).  Queries: `s "function-try-block"`, `s "function try block"`, `p "function-try-block"`, `p "except.handle"`, `s "handler constructor member constexpr"`, `q "constexpr exception handler member constructor lifetime"`.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

**Summary:** Referring to a member in a constructor's function-try-block handler is not rejected in constant evaluation

**Description:** The program below has core-language undefined behavior ([except.handle]; D4277R1 category `ub:except.handle.handler.ctor.dtor`), so the static_assert's condition is not a core constant expression ([expr.const]) and must be rejected; it is accepted.  A never-initialized or already-destroyed member or base is rejected; an initialized scalar member, and `&y`, are accepted.

    struct S {
      int x;
      constexpr S(bool b) try : x(1) { if (b) throw 0; }
      catch (...) { int y = x; (void)y; }   // UB [except.handle]/10
    };
    constexpr bool f() { try { S s(true); } catch (...) {} return true; }
    static_assert(f());

Found by the D4277R1 per-UB review (2026-10-12).
