---
id: 0415-ipa-pta-interposable-callee
subject: 'ipa-pta: Do not use the body of an interposable callee'
depends: []
regenerates: []
fixes: [gcc-897]
---

## Rationale

GCC-897 (no upstream report found).  Not a contracts bug.  IPA points-to
analysis (`-fipa-pta`) modelled a direct call by the constraints of the
callee's body whenever the callee had one, without looking at its
availability.  For a weak definition the linker may pick another
definition, which may let an argument escape where the body seen does
not, so a caller's object was found not to escape and a later read of it
was folded to a stale value (wrong code).  The same held for an exported
function under `-fPIC` with semantic interposition, and for a `noipa`
function, whose body is documented to be treated as unavailable when
optimizing its callers.  The other IPA passes that derive facts for
callers all check availability.

`fndecl_call_interposable_p` is true when the callee's availability
(through aliases and thunks) is at most `AVAIL_INTERPOSABLE`, and the
three places that model a direct call -- the argument and return value
constraints in `find_func_aliases_for_call`, the use/clobber constraints
in `find_func_clobbers` and the call use/clobber sets computed in
`ipa_pta_execute` -- then treat it as a call to an external function: its
arguments escape, and ESCAPED is used and clobbered.  The callee's body
is still analysed, its parameters pointing to NONLOCAL as for any
exported function.  Indirect calls that may reach such a function still
use its body's constraints.  Code without interposable direct callees is
unchanged.

`gcc.dg/ipa/ipa-pta-18.c` checked that the arguments of a call of a
`noipa` function were assigned to its parameters; they are no longer,
and it now checks that they are not.

## Compile gap

None.  Everything named is upstream's; the tests use no contracts.

## Contents

- gcc/gimple-ssa-pta-constraints.cc : *
- gcc/tree-ssa-structalias.cc : *
- gcc/tree-ssa-structalias.h : *
- gcc/testsuite/gcc.dg/ipa/ipa-pta-18.c : *
- gcc/testsuite/gcc.dg/ipa/ipa-pta-21.c : *
- gcc/testsuite/gcc.dg/ipa/ipa-pta-22.c : *
- gcc/testsuite/gcc.dg/ipa/ipa-pta-23.c : *
