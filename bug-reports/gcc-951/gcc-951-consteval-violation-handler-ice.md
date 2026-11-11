# GCC-951: a consteval ::handle_contract_violation ICEs in build_call_a

**Status:** Open (upstream); fixed here
**Resolved by:** [a46355faede3](https://github.com/notadragon/gnu_gcc/commit/a46355faede38105b67afc3e76f1d6c5083516d7) (`check_handle_contract_violation`,
which rejects an inline replacement handler)
**Component:** c++
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`
**Upstream Link:** [PR128202](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128202)
(filed 2026-10-27 by someone else; found by the Phase 5.5 Bugzilla sweep of
2026-10-29)
**Affects:** measured 2026-10-29 -- stock trunk `17.0.0 20261029`
(`6aae4093ee77`): ICE with plain `-fcontracts` and with
`-fcontract-evaluation-semantic=observe` or `enforce`; no ICE with `ignore`
or `quick_enforce`, which never call the handler.  This branch: rejected at
the declaration, all four semantics.
**Verify case:** `GCC-951` (compile, plain `-fcontracts`).

## Reproducer

[`gcc-951-consteval-violation-handler-ice.cpp`](gcc-951-consteval-violation-handler-ice.cpp),
the reporter's program:

```cpp
#include <contracts>
consteval void handle_contract_violation (const std::contracts::contract_violation &) {}
int main () { contract_assert (false); }
```

```
In function 'void __tu_has_violation(const __builtin_contract_violation_type&, short unsigned int)':
internal compiler error: in build_call_a, at cp/call.cc:392
```

## Why

`maybe_emit_violation_handler_wrappers` builds `__tu_has_violation`, whose
body calls `::handle_contract_violation`.  When the user's declaration is
consteval that call is an immediate invocation from a non-immediate
function, and `build_call_a` asserts on it.

The program is ill-formed: a consteval (or constexpr) function is
implicitly inline ([dcl.constexpr]), and a replacement function shall not
be declared inline ([dcl.fct.def.replace]).  Stock trunk has no check for
any of the replacement-function rules on this handler, so the consteval
spelling reaches code generation.

**The constexpr spelling is a silent accepts-invalid on stock**, measured
the same day: it compiles cleanly, the inline definition is never emitted,
and the object file references `handle_contract_violation` as undefined --
so the program links against the library's default handler and the user's
handler is silently not the one called.

## Fixed here

`check_handle_contract_violation` (called from `grokfndecl` for a
`::handle_contract_violation` declaration) rejects
`DECL_DECLARED_INLINE_P`, which consteval and constexpr both set:

```
error: '::handle_contract_violation' must not be declared 'inline'
```

The diagnostic names `inline` rather than the `consteval` the user wrote; it
is correct (the function is inline) but could say why.  Pinned by
`g++.dg/contracts/cpp26/pr128202.C` (consteval) and
`violation-handler-constexpr.C` (constexpr), both at the observe semantic.
