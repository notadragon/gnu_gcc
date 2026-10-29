# GCC-651: an unexpanded parameter pack in a contract predicate is not diagnosed

**Status:** Open (upstream); fixed here
**Resolved by:** [a6703a3b216b](https://github.com/notadragon/gnu_gcc/commit/a6703a3b216b30803d15fbbeb2b3dce4ba748e45)
**Component:** c++ (contracts, variadic templates)
**Keywords (ours -- upstream sets its own):** `accepts-invalid`
**Upstream Link:** None found (searched 2026-10-06: Bugzilla quicksearch for
`contract unexpanded parameter pack`, `contract_assert parameter pack not
expanded`, `type_pack_expansion contract`).
**Affects:** measured 2026-10-06 against stock trunk (`17.0.0 20260922`)
and this branch before the fix: identical.

**Bugzilla Summary:** [c++26][contracts] unexpanded parameter pack in a
contract predicate is not diagnosed

## Reproducer

[`gcc-651-contract-unexpanded-pack.cpp`](gcc-651-contract-unexpanded-pack.cpp),
at `-std=c++26 -fcontracts -c`:

    template <class... T> int f1 (T... xs) pre (xs > 0) { return 0; }
    template <class... T> int f3 (T... xs) { contract_assert (xs > 0); return 0; }
    int a = f1 (1, 2);

    sorry, unimplemented: use of 'type_pack_expansion' in template

`f3` compiles with no diagnostic at all.  Any other expression naming `xs`
unexpanded is rejected in the template ("parameter packs not expanded with
'...'").  Clang (with contracts) rejects all three.

## Notes

A contract predicate never passes through `check_for_bare_parameter_packs`,
which an expression statement, an initializer or a return value does.  On
this branch the predicate's conversion to bool (`finish_contract_condition`)
now makes the check, which also covers a P3098 init-capture pack named
unexpanded in a postcondition (GCC-639).

## Found by

Root-causing GCC-638 and GCC-639 (P3098 captures naming packs), 2026-10-06.
