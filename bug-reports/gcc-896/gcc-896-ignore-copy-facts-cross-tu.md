# GCC-896: facts from a copy whose contract assertion is ignored reach callers that run another TU's copy

**Status:** Open (upstream); fixed here
**Resolved by:** [76208014c18f](https://github.com/notadragon/gnu_gcc/commit/76208014c18f720e81e3c9a908704a916cc0b2de) [5fccf4269c80](https://github.com/notadragon/gnu_gcc/commit/5fccf4269c807dec5d3b87d9bfe87c9cbf64fa41)
**Component:** c++ (front end) and ipa (middle end)
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** None found (searched 2026-10-21: Bugzilla REST quicksearch
`contract nothrow`, `contracts ignore terminate`, and every bug whose
summary carries `[contracts]`; nothing about a contract assertion's semantic
differing between the copies of an inline function).  The general case --
IPA trusting the body of a vague-linkage function another TU may provide --
is PR121936; PR124088 is about stock's `-fcontracts-conservative-ipa`
(removed on this branch), which does not help here -- filing owed
**Affects:** measured 2026-10-21 -- 16.1.0, 16.2.0, the Compiler Explorer
trunk build `17.0.0 20261019` (`1f9dc1fbeb0`), the stock trunk build
`17.0.0 20260922` (`gnu_gcc-trunk-install`), and this branch (the install
of `17.0.0 20261019` and the Space build of `1ff5a935dcf`); -O0 to -O3,
with stock's `-fcontracts-conservative-ipa` (the default there).  Clang (our fork):
the nothrow form reproduces (llvm_llvm-project CLANG-824); the memory form
does not.
**Regression tests:** `g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-*.C`
(seven)

## Reproducer

Two translation units define the same inline function `f`; TU I ignores
its contract assertion, TU E evaluates it (`observe` or `enforce`) with a
violation handler that throws, and the linker keeps E's copy (`e.o` is
linked first).  [`i.cc`](i.cc), [`e.cc`](e.cc), [`m.cc`](m.cc), commands in
each file's header:

    $ ./a.out
    terminate called after throwing an instance of 'int'
    $ ./a.out x
    caught dtors=0

Expected `use_catch=-1` (TU I's `use_catch` catches the exception) and
`caught dtors=1` (`use_dtor`'s local is destroyed during unwinding).  The
same with `f` a function template (implicit instantiations are comdat too),
and with `f` calling an inline function defined after it, so that the front
end does not know `f` cannot throw: the middle end finds it instead.

A second form needs no exception.  [`pure-i.cc`](pure-i.cc),
[`pure-e.cc`](pure-e.cc), [`pure-m.cc`](pure-m.cc): TU I's `use_g` stores
1 to `g`, calls `f`, and adds `g` to the result; E's copy observes the
assertion and its handler sets `g` to 5:

    $ ./a.out
    r=-4 calls=1

Expected `r=0 calls=1`: the handler ran, so `g` is 5 after the call.

Both are wrong for every choice of evaluation semantic.  The semantic of a
contract assertion may differ between evaluations, including between the
copies of an inline function in different TUs ([basic.contract.eval]; P2900
section 3.5.11: mixed mode is not an ODR violation).  The handler did run --
the exception reached `main`, `calls` is 1 -- so the call evaluated the
assertion; a program in which it ran must unwind through `use_catch`'s
handler and `use_dtor`'s cleanup, and must see the handler's store.

## Cause

GCC derives facts about `f` from TU I's copy, where the ignored assertion
leaves a body with no call and no memory access, and applies them to the
out-of-line call, which runs E's copy.  Every producer below treats the
comdat body as the definitive one (`cgraph_node::get_availability`,
`gcc/cgraph.cc:2823-2838`: inline and comdat functions are
`AVAIL_AVAILABLE`, "not meaningful to enforce any sane behavior on
replacing inline function by different body"):

* nothrow, front end: `finish_function` (`gcc/cp/decl.cc:21258-21265`)
  sets `TREE_NOTHROW` when no call in the body may throw, guarded by
  `decl_replaceable_p`, which is false for comdat.  Fires at -O0.
* nothrow, middle end: `pass_nothrow` (`gcc/ipa-pure-const.cc:2354`), local
  and IPA pure-const, and `set_nothrow_function_flags` on the RTL
  (`gcc/except.cc:1976`, `get_availability () >= AVAIL_AVAILABLE`).  The
  RTL one fires at -O0 when `f` is expanded before its caller.
* const/pure: local pure-const finds `f` const
  (`ipa_make_function_const`, `gcc/ipa-pure-const.cc:1500`); the weakening
  to pure for a callee that may not bind to the current definition
  (`gcc/ipa-pure-const.cc:510-519`) does not apply to comdat.
* mod-ref: `f`'s summary records no stores (`gcc/ipa-modref.cc`).  Either
  pure-const or mod-ref alone folds the reload of `g`
  (`-fno-ipa-pure-const`, `-fno-ipa-modref`).

Stock's `-fcontracts-conservative-ipa` only hides the terminating
semantics' reaction behind a `noipa` wrapper; it does nothing for an
ignored assertion, whose body has no reaction to hide.

The direction is the reverse of GCC-894 (this branch only, fixed by the
same change): there the copy with *more* effects (an `enforce` check's
noreturn reaction) gives facts that a copy with fewer effects breaks.
Both break the same rule.

## On this branch

The same, with the semantics selected per assertion
(`-fcontract-configuration-file=`), and for a C99 inline function in C
compiled with `-fexceptions` and unwound by a C++ violation handler (the
cleanup of an `__attribute__ ((cleanup))` local is skipped), and in its
memory form.  The fix is the refinable-body design
(`notadragon_wg21: src/pubs/impl/p3850impl/docs/superpowers/specs/2026-10-21-refinable-body-design.md`):
a function holding a contract evaluation whose semantic is not settled is
marked, and `get_availability` treats it as interposable for callers that
do not inline it.  Fixed on this branch: the middle end's mark
(`0420-refinable-body`), the C++ front end marking every function that
holds a contract assertion or calls a contract wrapper
(`0430-refinable-body-contracts`; `-fcontracts-conservative-ipa` removed),
the C front end marking C contract assertions (`5000-p4299`), and P3400
labels that settle an evaluation (`4230-p3400-facet-allowed-semantics`).
The seven tests below pass.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[contracts] an inline function's ignored contract assertion makes it nothrow/const for callers that run another TU's copy` |

````
Two TUs define the same inline function; one ignores its contract
assertion, the other evaluates it with a violation handler that throws (or
has side effects), and the linker keeps the evaluating copy.  P2900 allows
the semantic to differ between the copies (mixed mode is not an ODR
violation).

// i.cc: g++ -std=c++26 -fcontracts -O2 -fcontract-evaluation-semantic=ignore -c i.cc
struct D { ~D (); };
[[gnu::noinline]] inline int f (int x) { contract_assert (x > 0); return x; }
int use_catch (int y) { try { return f (y); } catch (...) { return -1; } }
int use_dtor (int y) { D d; return f (y); }

// e.cc: g++ -std=c++26 -fcontracts -O2 -fcontract-evaluation-semantic=observe -c e.cc
#include <contracts>
[[gnu::noinline]] inline int f (int x) { contract_assert (x > 0); return x; }
int (*fp_e) (int) = f;
void handle_contract_violation (const std::contracts::contract_violation &)
{ throw 42; }

// m.cc: g++ -std=c++26 -fcontracts -O2 -c m.cc; g++ -fcontracts m.o e.o i.o
#include <cstdio>
int use_catch (int); int use_dtor (int);
int dtors;
struct D { ~D (); };
D::~D () { ++dtors; }
int main (int argc, char **)
{
  if (argc == 1) { std::printf ("use_catch=%d\n", use_catch (-5)); return 0; }
  try { use_dtor (-5); } catch (int) { std::printf ("caught dtors=%d\n", dtors); }
}

$ ./a.out
terminate called after throwing an instance of 'int'
$ ./a.out x
caught dtors=0

Expected "use_catch=-1" and "caught dtors=1".  In i.cc the ignored
assertion leaves f with no call, so finish_function marks it TREE_NOTHROW
(decl_replaceable_p is false for comdat), as do pass_nothrow and the RTL
nothrow discovery, and use_catch/use_dtor lose their handler and cleanup.
With a handler that writes a global instead of throwing, f is also found
const and its mod-ref summary says it stores nothing, so a caller's reload
of that global after the call is folded.  -fcontracts-conservative-ipa does
not help.  This is PR121936's problem with a contract assertion's semantic
as the difference between the copies.
````

## Notes (ours, not for the report)

No `verify-cases.txt` row: the case needs three invocations with different
`-fcontract-evaluation-semantic=` values, and `verify.sh` measures one
invocation per row.

Regression tests (multi-TU, `-O2`, with the semantics selected by
`refinable-body-ignore-replaceable.json`):

| Test | Shows |
|---|---|
| `-catch.C` | inline function, front-end nothrow: `std::terminate` instead of the caller's `catch` |
| `-dtor.C` | the same: a local's destructor is skipped while unwinding |
| `-late.C` | nothrow found by the middle end (the front end cannot see it): destructor skipped |
| `-template.C` | function template instantiation: `std::terminate` instead of `catch` |
| `-pure.C` | const/mod-ref: the handler's store is not reloaded (`-4`, not `0`) |
| `-c99.C` | C99 inline in C with `-fexceptions`, C++ handler: a `cleanup` local skipped |
| `-c99-pure.C` | C99 inline: the handler's store is not reloaded |

Eliding or merging calls to the ignore copy (`f (y);` with the result
unused is deleted; `f (y) + f (y)` makes one call) is not a defect: it
behaves as if those evaluations were ignored, which is a permitted choice.

## Found by

Research for the refinable-body design (2026-10-21, direction B);
filed by M1-GCC, which found the memory form (the research had tried a
handler that reads `g`, which saw the right value; a handler that writes
`g` shows the fold).
