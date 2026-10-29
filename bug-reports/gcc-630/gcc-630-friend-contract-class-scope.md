# GCC-630: a friend's contract predicate cannot name members of its class

**Status:** Open (upstream); fixed here
**Resolved by:** [8a434a321f10](https://github.com/notadragon/gnu_gcc/commit/8a434a321f101059042dc0feabae722d0b18ebda)
**Component:** c++ (contracts, name lookup)
**Keywords (ours -- upstream sets its own):** `rejects-valid`
**Upstream Link:** None found (searched 2026-10-01: the 50 open Bugzilla
bugs with "contracts" in the summary; PR127291 is about friends but concerns
matching contract assertions across declarations).
**Affects:** measured 2026-10-01 against stock trunk (`17.0.0 20260922`),
16.1.0 and this branch: identical on all three.  15.3.0 has no contracts.

**Bugzilla Summary:** [c++26][contracts] a friend function's contract
predicate does not find members of the class it is declared in

## Reproducer

[`gcc-630-friend-contract-class-scope.cpp`](gcc-630-friend-contract-class-scope.cpp),
at `-std=c++26 -fcontracts`:

    struct S
    {
      static constexpr int limit = 10;
      int m (int x) pre (x != limit) { return x; }
      friend int declared (int x) pre (x != limit);
      friend int defined (int x) pre (x != limit) { return x; }
    };
    int declared (int x) { return x; }

    error: 'limit' was not declared in this scope   (the declared friend)
    error: 'limit' was not declared in this scope   (the defined friend)

The member function's precondition is accepted.  A friend declared in a
class is in that class's scope, so an unqualified name in its declarator --
its contract specifiers included -- is looked up there, as it is for a
parameter type (`friend void f (T)` with `T` a member typedef works), and a
friend defined in the class is looked up as a member function definition
is ([basic.lookup.unqual]).  Clang (with contracts) and the EDG front end
accept both friends.  Where `limit` is declared makes no difference: it is
rejected before and after the friend.  `S::limit` is accepted.

## Notes

The error is reported in the friend's function context ("In function 'int
declared(int)'"), consistent with the predicate being parsed or bound once
the class scope is no longer active (contract predicates of functions
declared in a class are deferred to the end of the class).  Not yet located
in the source.

## Found by

EDG's contracts suite, once the EDG fork passed contract assertions through
to g++ (`tests/tests/contracts/codegen/members.sft.cpp` in edgcpp_compiler).

## Fixed here

The class-end late parse of contract specifiers switched to the function's `DECL_CONTEXT`, which for a friend is its namespace, so the class's members were out of scope.  The queue of deferred contracts now records the class the function was declared in (as the queues of default arguments and noexcept-specifiers already did), and the predicates are parsed in its scope.  Regression test: `g++.dg/contracts/cpp26/friend-contract-class-scope.C`.
