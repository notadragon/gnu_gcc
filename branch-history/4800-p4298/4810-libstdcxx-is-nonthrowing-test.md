---
id: 4810-libstdcxx-is-nonthrowing-test
subject: 'libstdc++: testsuite: P4298: exercise std::contracts::is_nonthrowing across its semantics'
depends: [4200-p3400-core, 4800-p4298]
regenerates: []
fixes: []
---

## Rationale

Exercise `std::contracts::is_nonthrowing` (a `4200-p3400-core` addition to
`<contracts>`) against every `evaluation_semantic` enumerator, including the
two `4800-p4298` adds (`noexcept_enforce`, `noexcept_observe`).  It needs its
own commit rather than living in either paper's own test tree because it is
a cross-paper regression check: `is_nonthrowing` is P3400's function, but a
classification of "every nonthrowing semantic" is not exhaustive, and so not
worth pinning, until P4298's two semantics exist alongside P3100's `assume`
and the base `ignore`/`quick_enforce`.  A single `static_assert` block
covering the whole enumeration belongs with the general libstdc++ testsuite
(`18_support/contracts/`) rather than either paper's g++.dg tree, matching
the file's own comment that the C++ library surface here is not otherwise
exercised from libstdc++ at all.

## Compile gap

None beyond its stated `depends`, checked against the diff rather than
assumed.  `is_nonthrowing` and the `evaluation_semantic_set`
class are added in one hunk of `libstdc++-v3/include/std/contracts`,
together with all three of `assume = 5`, `noexcept_observe = 6` and
`noexcept_enforce = 7`.  The `assume` enumerator's comment says "P3100", but
the enumerator itself sits inside `4200-p3400-core`'s claimed segment (its
own `/is_nonthrowing/` and `/evaluation_semantic_set/` selectors pick up the
whole hunk), not `6000-p3100-core`'s -- so `evaluation_semantic::assume` is
already available at this test's first listed dependency and does not also
require `6000-p3100-core`, a commit thousands of bands later.  The
`noexcept_enforce`/`noexcept_observe` enumerators are `4800-p4298`'s, this
test's other dependency.  Both dependencies precede this commit in band
order, so `depends: [4200-p3400-core, 4800-p4298]` is complete and correctly
ordered; compare `4650-reentrancy-everything`, whose Compile gap records the
case where it is not.

## Contents

- libstdc++-v3/testsuite/18_support/contracts/is_nonthrowing.cc : *
