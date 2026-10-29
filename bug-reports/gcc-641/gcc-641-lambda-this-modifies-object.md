# GCC-641: a lambda in a contract predicate may modify `*this` through its capture

**Status:** Open (upstream); fixed here
**Resolved by:** [bee70cf7e689](https://github.com/notadragon/gnu_gcc/commit/bee70cf7e689eccdcc184b10a0e9c75d4fb2d6d4)
**Component:** c++ (contracts, lambdas)
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contracts lambda this const`, `contract predicate this lambda modify`,
`contracts constification this lambda`, and the list of open c++ bugs whose
summary contains "contract"; PR117436 is about the opposite, a data member
in a predicate lambda being rejected).
**Affects:** measured 2026-10-06 against stock trunk (`17.0.0 20261005`) and
this branch: both accept.

**Bugzilla Summary:** [c++26][contracts] a lambda in a contract predicate can
modify the object through a captured this

## Reproducer

[`gcc-641-lambda-this-modifies-object.cpp`](gcc-641-lambda-this-modifies-object.cpp),
at `-std=c++26 -fcontracts -fsyntax-only`:

    struct S {
      int m;
      void f () pre ([this] { return ++m > 0; } ());
      void g () { contract_assert ([&] { return ++m > 0; } ()); }
    };

Both compile.  P2900 [expr.prim.this]: where `this` appears within a
contract predicate its type is pointer to const, and a lambda's `this`
names the enclosing function's, so `++m` modifies a const object.  Clang
(with contracts) and the EDG front end reject both; a read (`m > 0`) is
fine.

## Found by

The EDG fork's EDG-26 (the divergence recorded from EDG's side), confirmed
against our GCC and Clang, 2026-10-03; re-measured 2026-10-06.

## Fixed here

`this' in a predicate was the plain pointer; only member accesses through current_class_ref were constified.  It is now a pointer to const in the predicate (finish_this_expr, maybe_resolve_dummy), and a lambda in a predicate capturing it from outside captures a pointer to const.  A lambda's copy of *this stays modifiable, as [expr.prim.id.unqual]'s example requires.
