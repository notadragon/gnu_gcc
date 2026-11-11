# GCC-806: handler effects are lost across a routed address or thread check that continues

**Kind:** deferred (a documented limitation) -- deferred by the user
2026-10-15 (DECISIONS.md E90 (b))
**Status:** Open, this branch only (routing a sanitizer check to the
contract-violation handler is a P3850 extension)
**Affects:** `-fsanitize=address` and `-fsanitize=thread` routed with
`-fsanitize-semantic=address:noexcept_observe` /
`thread:noexcept_observe` (a semantic that calls the handler and
continues), at -O1 and above; measured 2026-10-15 on the install of
`bc86ea4d74c`
**Watch tests:** `g++.dg/asan/open-bug-routed-handler-effects-late.C`,
`g++.dg/tsan/open-bug-routed-handler-effects-late.C` (output xfailed, -O2
variants only)
**Clang:** the same (llvm_llvm-project CLANG-761, also deferred by E90,
for local-bounds, address, memory and thread)

## Symptom

    static int reports;
    void handle_contract_violation (const std::contracts::contract_violation &)
    { ++reports; std::puts ("H"); }
    ...
    int before = reports;
    volatile int x = h[i];                        // heap, one past the end
    std::printf ("address %d\n", reports - before);

With `-O2 -fsanitize=address -fsanitize-semantic=address:noexcept_observe
-fcontracts-p3100 -fcontracts-p4298` the handler runs (it prints "H") and
the program continues, but it prints `address 0`; at -O0, `address 1`.  The
same for a data race under `-fsanitize=thread`.

## Cause

The address and thread checks are inserted by `pass_asan` and `pass_tsan`,
which run after the optimizers that forward loads (FRE, PRE): those passes
saw no call at the access, so they reused the value of `reports` loaded
before it.  GCC-805 fixed the checks that exist before those passes run
(the front end's and `pass_ubsan`'s internal functions); a check inserted
afterwards cannot be fixed by how its call is declared.

## Disposition

E90 (b): a documented limitation.  After a routed check that the middle end
inserts late reports and continues, the program goes on to perform the
undefined access, and the handler's side effects on objects the program
then reads are not guaranteed visible at -O1 and above.  Documented under
`-fsanitize-semantic=` in `gcc/doc/invoke.texi` and in
`p3100-gcc-implementation.md`.  The rejected alternative, (a), was to
instrument before the optimizers when such a check is routed to a
continuing semantic.
