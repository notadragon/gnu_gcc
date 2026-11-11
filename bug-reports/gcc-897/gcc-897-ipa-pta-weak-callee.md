# GCC-897: `-fipa-pta` uses the body of a weak (interposable) callee for calls of it

**Status:** Fixed here
**Resolved by:** `0415-ipa-pta-interposable-callee`
**Component:** ipa
**Keywords (ours -- upstream sets its own):** `wrong-code`
**Upstream Link:** None found (searched 2026-10-21: Bugzilla quicksearch
`ipa-pta weak`, `ipa-pta interposable`, `ipa-pta interposition`,
`ipa-pta noipa`; every summary containing `ipa-pta`, `IPA PTA` or
`fipa-pta` read -- PR38261 is about tests failing under `-fpic`, not this)
-- filing owed
**Affects:** measured 2026-10-21 -- 13.4.0, 16.1.0, 16.2.0, the Compiler
Explorer trunk build `17.0.0 20261019` (`1f9dc1fbeb0`) and the stock trunk
build `17.0.0 20260922` (`gnu_gcc-trunk-install`), at `-O2 -fipa-pta`;
`-O2` alone is correct.  The same modelling ignores `-fPIC` semantic
interposition (an exported function's body is used for its callers) and
the `noipa` attribute (whose documentation says the body is treated as
unavailable when optimizing callers).

## Reproducer

[`gcc-897-ipa-pta-weak-callee.c`](gcc-897-ipa-pta-weak-callee.c), at
`-O2 -fipa-pta -Wl,--defsym=keep=keep_escape`, run:

    use () = 1 (2 expected)

The `--defsym` stands in for the ordinary case, a strong definition of
`keep` in another translation unit that lets its argument escape, which
the linker picks over the weak one.  With two files (`keep` weak in the
first, strong in the second) the result is the same.

## Cause

IPA points-to analysis (`-fipa-pta`, off at every `-O` level) models a
direct call by the constraints of the callee's body whenever the callee
has one: the arguments are assigned to its parameters, and the caller
uses and clobbers what the callee does (`find_func_aliases_for_call` and
`find_func_clobbers`, `gcc/gimple-ssa-pta-constraints.cc`; the call
use/clobber sets in `ipa_pta_execute`, `gcc/tree-ssa-structalias.cc`).
None of the three consults `cgraph_node::get_availability`, so a body
that is `AVAIL_INTERPOSABLE` -- a weak definition, an exported function
under `-fPIC` with semantic interposition, a `noipa` function -- is used
like any other.  Every other IPA pass that derives facts for callers
(pure-const, mod-ref, ipa-reference, return ranges) checks availability.

## Our Fix

`fndecl_call_interposable_p` (`tree-ssa-structalias.cc`) is true when the
callee's `function_symbol` availability is at most `AVAIL_INTERPOSABLE`;
the three sites then model the call as a call to an external function:
the arguments escape, and ESCAPED is used and clobbered.  The callee's
body is still analysed, with its parameters pointing to NONLOCAL as for
any exported function.  Indirect calls that may reach such a function
still use its body's constraints.

Tests: `gcc.dg/ipa/ipa-pta-21.c` (weak and `noipa` callees; strong
external and static controls), `ipa-pta-22.c` (`-fPIC
-fsemantic-interposition`; hidden control), `ipa-pta-23.c` (this
reproducer, run), and `g++.dg/ipa/refinable-pta.C`.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `ipa` |
| Version | `16.2.0` |
| Severity | `normal` |
| Summary | `-fipa-pta uses the body of a weak function for calls of it` |

````
/* t1.c */
int g;
void opaque (void);
__attribute__((weak, noinline)) void keep (int *p) { g = 1; }
__attribute__((noinline)) int use (void)
{
  int x = 1;
  keep (&x);
  opaque ();
  return x;
}
int main (void) { if (use () != 2) __builtin_abort (); }

/* t2.c */
static int *saved;
void keep (int *p) { saved = p; }
void opaque (void) { *saved = 2; }

$ gcc -O2 t1.c t2.c && ./a.out
$ gcc -O2 -fipa-pta t1.c t2.c && ./a.out
Aborted

The linker picks the strong definition of keep from t2.c, which lets &x
escape, but IPA-PTA models the call in use() by the weak body in t1.c,
finds that x does not escape and folds "return x" to "return 1".  None of
the call modelling in IPA-PTA looks at the callee's availability, so the
same happens for an exported function under -fPIC (semantic
interposition) and for a noipa function.
````

## Found by

The refinable-body program's M2-GCC probe (audit item C18 / 27 in
`notadragon_wg21` `refinable-body-gcc-audit.md`): checking whether
IPA-PTA honours the refinable mark showed it honours no interposition at
all.
