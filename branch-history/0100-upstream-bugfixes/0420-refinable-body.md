---
id: 0420-refinable-body
subject: 'ipa: Add a refinable-body mark for definitions that may differ across TUs'
depends: [0415-ipa-pta-interposable-callee]
regenerates: []
fixes: []
---

## Rationale

A middle-end mark for function bodies that may differ, in ways that
matter, from the other definitions of the same symbol the linker may
choose -- for example because each translation unit made its own choice
where the language lets the behavior of the same source differ between
evaluations (PR121936 discusses the problem for vague-linkage functions).
Such a body is a valid definition of the function, so it may be inlined,
cloned and split; but a call of the symbol that is not inlined must not
rely on facts derived from it.

The mark is the internal `"refinable body"` attribute (a space in the name,
so it cannot be spelled in source), streamed by LTO with the decl.
`cgraph_node::refinable_p` is true when the function whose body it is (the
ultimate alias target) carries the attribute and the symbol may bind to
another definition: an external inline body (C99 inline, gnu_inline,
explicit instantiation declaration), a COMDAT or weak definition, or an
otherwise replaceable one.  `cgraph_node::get_availability` then returns
`AVAIL_INTERPOSABLE`, so every interposition-aware producer and consumer
of facts becomes conservative: return ranges and `returns_nonnull`,
const/pure/noreturn/malloc/nothrow discovery at GIMPLE and RTL level,
mod-ref, ipa-reference.  Users that copy the body pass a new `copy_body`
argument (to `get_availability`, `ultimate_alias_target`,
`function_symbol` and `function_or_virtual_thunk_symbol`) and see the
availability the function would have without the mark: the early and IPA
inliners (including `always_inline`), IPA-CP, IPA-SRA, caller collection
for redirection to clones, speculative devirtualization, the inlining
benefit estimate and locality cloning; and LTO symbol merging keeps a
preempted refinable body for inlining.  The visibility pass creates no
`.localalias` for a refinable body's recursive calls.

Inlining a body carrying the attribute adds it to the function it is
inlined into (in `inline_call`, which every inlined edge goes through), so
the mark follows the code; `refinable_p` decides by linkage whether it
matters.  Target clones keep the attribute; other clones inherit it with
the decl.  `set_pure_flag` no longer sets the flag on SIMD clones whose
own availability refuses it.  Symbol table dumps show "(refinable body)".

Nothing sets the attribute yet except `--param=ipa-refinable-inline=1`,
a testing aid that marks every function declared inline; without it no
availability, and so no generated code, changes.  IPA-PTA honours the
mark through availability, as it does weak definitions
(`g++.dg/ipa/refinable-pta.C`).

## Compile gap

None.  Everything named is upstream's; the tests use no contracts.

## Contents

- gcc/cgraph.cc : *
- gcc/cgraph.h : *
- gcc/cgraphclones.cc : *
- gcc/cgraphunit.cc : /param_ipa_refinable_inline/
- gcc/doc/params.texi : *
- gcc/ipa-cp.cc : *
- gcc/ipa-devirt.cc : *
- gcc/ipa-fnsummary.cc : *
- gcc/ipa-inline-transform.cc : *
- gcc/ipa-inline.cc : *
- gcc/ipa-locality-cloning.cc : *
- gcc/ipa-profile.cc : *
- gcc/ipa-sra.cc : *
- gcc/ipa-visibility.cc : *
- gcc/lto/lto-symtab.cc : *
- gcc/params.opt : *
- gcc/symtab.cc : *
- gcc/testsuite/g++.dg/ipa/refinable-1-ctl.C : *
- gcc/testsuite/g++.dg/ipa/refinable-1.C : *
- gcc/testsuite/g++.dg/ipa/refinable-2.C : *
- gcc/testsuite/g++.dg/ipa/refinable-cp.C : *
- gcc/testsuite/g++.dg/ipa/refinable-extern-ctl.C : *
- gcc/testsuite/g++.dg/ipa/refinable-extern.C : *
- gcc/testsuite/g++.dg/ipa/refinable-icf.C : *
- gcc/testsuite/g++.dg/ipa/refinable-inline.C : *
- gcc/testsuite/g++.dg/ipa/refinable-nothrow-rtl.C : *
- gcc/testsuite/g++.dg/ipa/refinable-nothrow.C : *
- gcc/testsuite/g++.dg/ipa/refinable-prop.C : *
- gcc/testsuite/g++.dg/ipa/refinable-pta.C : *
- gcc/testsuite/g++.dg/ipa/refinable-sra.C : *
- gcc/testsuite/g++.dg/lto/refinable-1_0.C : *
- gcc/testsuite/g++.dg/lto/refinable-1_1.C : *
- gcc/testsuite/g++.dg/lto/refinable-2.h : *
- gcc/testsuite/g++.dg/lto/refinable-2_0.C : *
- gcc/testsuite/g++.dg/lto/refinable-2_1.C : *
- gcc/testsuite/gcc.dg/ipa/refinable-1-ctl.c : *
- gcc/testsuite/gcc.dg/ipa/refinable-1.c : *
- gcc/testsuite/gcc.dg/lto/refinable-1_0.c : *
- gcc/testsuite/gcc.dg/lto/refinable-1_1.c : *
