# GCC-996: after a function with `optimize ("O2")`, the functions compiled at -O1 keep -O2's alignment

**Status:** Open
**Component:** middle-end (`gcc/toplev.cc`, `parse_N_M`)
**Keywords (ours -- upstream sets its own):** missed-optimization
(code size), wrong output with respect to the documented options
**Upstream Link:** None found (searched 2026-11-08: Bugzilla quicksearch
`optimize attribute alignment`, `falign optimize attribute`, `p2align
optimize attribute`, `parse_N_M`, `align_loops optimize attribute`,
`attribute optimize falign-functions`, `optimize attribute falign-loops`,
`pragma GCC optimize alignment`, `alignment leaks optimize attribute`, and
the summaries containing `align` filed since 2024-06-01; PR88345 (-Os
overriding `-falign-functions=N`), PR67435 (an align-loops attribute) and
PR110724 are other alignment issues, none is this) -- filing owed
**Affects:** x86_64, measured 2026-11-08 with the stock compilers in
`~/repos/compilers`: `gcc-15.3.0`, `gcc-16.1.0`, `gcc-16.2.0` and the trunk
nightly `17.0.0 20261103 (experimental)` (`6d9a79665c6`) reproduce it;
`gcc-13.4.0` and `gcc-14.4.0` do not.  Not bisected.  Not a contracts bug;
found by PO2-GCC of the D4405 follow-up program, whose proof shadows,
optimized at level 2 in a -O1 compilation, changed the alignment of the
real functions compiled after them.

## Symptom

In a -O1 compilation, a function with `__attribute__ ((optimize ("O2")))`
(or under `#pragma GCC optimize ("O2")`) is compiled with -O2's
`-falign-functions`, `-falign-jumps`, `-falign-loops` and
`-falign-labels`, as it should be; but every function compiled after it
that sets no alignment of its own -- every plain -O1 function -- is then
aligned as at -O2 too.  -O1 does not enable those options, so nothing
resets the alignment for them.

Reproducer: [`gcc-996-align.c`](gcc-996-align.c), `gcc -O1 -S`.  With
15.3.0 and later, `g` gets `.p2align 4` before its label and `.p2align
4,,10` / `.p2align 3` before its loop, exactly as `f`; 13.4.0 and 14.4.0
give `g` no `.p2align`, and neither does any version when the attribute is
removed from `f`.  The order of the two definitions in the source does not
matter (cgraph compiles `f` first either way).

[`gcc-996-align-run.c`](gcc-996-align-run.c) shows it at run time, for
`verify.sh`: eight one-instruction -O1 functions after `f`, and how many
of them start on a 16-byte boundary.  15.3.0, 16.2.0 and trunk print
`aligned=8/8`; 13.4.0 and 14.4.0 print `aligned=1/8`, as does trunk
without the attribute.

## Cause

`invoke_set_current_function_hook` (`function.cc`) restores the function's
optimization options and then calls `parse_alignment_opts`
(`toplev.cc`), which runs `parse_N_M` for each of `str_align_loops`,
`str_align_jumps`, `str_align_labels` and `str_align_functions` into the
globals `align_loops`, `align_jumps`, `align_labels` and
`align_functions`.  `parse_N_M` does nothing when the string is NULL:

    static void
    parse_N_M (const char *flag, align_flags &a)
    {
      if (flag)
        {
          ...
        }
    }

At -O1 on x86 the `-falign-*` flags are off, so `ix86_default_align`
supplies no string and the four strings are NULL; the globals keep what the
previous function -- the -O2 one -- set.  At -O2 the strings are set and
each function overwrites the globals, which is why an all-optimized or
all-attribute-free unit never shows it.  `parse_N_M` itself has been
unchanged since GCC 9 (it is the same in 14.1.0 and on trunk), so what
changed in GCC 15 is elsewhere, presumably in how the globals are read;
it was not looked for.

## This branch

Reproduces on this branch exactly as on stock for user code.  The D4405
proof machinery works around it for its own functions: the optimization
node of the proof level keeps the command line's `-falign-*` options and
strings (`c_contract_proof_optimization` in
`gcc/c-family/contracts-prove-shadow.cc`), since analysis-only functions
are never expanded and must not change the alignment the real functions
get.  Without that, a -O1 or -Og compilation with proof shadows at level 2
emitted its real functions with -O2's alignment (found by the
`p4405-prove-optimize-same-code-*` tests).  The watch test is
`gcc.dg/contracts/open-bug-optimize-attribute-alignment.c`.

## Draft upstream report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `middle-end` |
| Version | `16.2.0` |
| Severity | `normal` |
| Summary | `[15/16/17 Regression] -O1 functions after an optimize("O2") function get -O2 alignment` |

````
With -O1, a function compiled after one with __attribute__((optimize
("O2"))) gets the -O2 -falign-functions/-falign-loops alignment:

  int sink (int);
  __attribute__ ((optimize ("O2"))) int f (int n)
  { int s = 0; for (int i = 0; i < n; ++i) s += sink (i); return s; }
  int g (int n)
  { int s = 0; for (int i = 0; i < n; ++i) s += sink (i); return s; }

gcc -O1 -S on x86_64: g has ".p2align 4" before its label and
".p2align 4,,10 / .p2align 3" before its loop, as f does.  Without the
attribute on f, g has no .p2align.  GCC 13.4 and 14.4 give g no
alignment; 15.3, 16.1, 16.2 and trunk (20261103) do.

invoke_set_current_function_hook calls parse_alignment_opts, and
parse_N_M (toplev.cc) leaves align_loops/align_jumps/align_labels/
align_functions unchanged when str_align_* is NULL, which it is at -O1
(the -falign-* flags are off there), so the values of the previous
function stay in effect.
````
