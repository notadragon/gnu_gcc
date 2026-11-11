---
id: 6900-cross-paper-options
subject: 'c-family: contracts: make every per-paper option imply -fcontracts'
depends:
  - 2900-umbrella-option
  - 1020-constexpr-side-effect
  - 2400-p3595
  - 3000-p3097
  - 3300-p3290
  - 3600-p3099
  - 4000-p3098
  - 4200-p3400-core
  - 4600-p4283
  - 4800-p4298
  - 4900-p4405
  - 5000-p4299
  - 5200-p4301
  - 5400-p4451
  - 6000-p3100-core
regenerates: []
fixes: [gcc-21]
---

## Rationale

What is left of the option registry once every line that could be divided
has gone home to the paper that owns it.  This commit is the residue: the
handful of places where one *line* names several papers at once, and a line
is the smallest thing the slicing can move.

Four such places, and one consequence of them.

* **The implication chain.**  `flag_contracts` gains
  `LangEnabledBy(C++ ObjC++, fcontracts-p3097 || fcontracts-p3098 || ...)`,
  a single boolean expression naming ten flags.  `.opt` has no line
  continuation -- `opt-gather.awk` drops an indented line and turns an
  unindented one into help text -- so the expression cannot be reformatted
  onto several lines, and cannot be cut.  It is left byte-for-byte as it
  is.

  Its companion is in `c_common_post_options`: the old
  `SET_OPTION_IF_UNSET (..., flag_contracts, cxx_dialect >= cxx26)` clobbers
  `flag_contracts` back to 0 in a pre-C++26 dialect, because a
  `LangEnabledBy` auto-handler does not mark the option as explicitly set.
  The guarded form only ever turns the flag on.  Neither half means anything
  until every per-paper option exists, and neither belongs to any one of
  them.

  A `LangEnabledBy` implication is also decided by whichever implying
  option comes last, which is wrong for options that are meant to add up:
  `-fcontracts-p3400 -fno-contracts-p3850` would leave `-fcontracts-p4451`
  off, and before C++26 a trailing `-fno-contracts-pNNNN` would turn
  `-fcontracts` off while another per-paper option is on.  So
  `c_common_post_options` decides both from the final values: P4451 is on
  when it is given or when P3400 or P3850 is on, and an explicit
  `-fno-contracts-p4451` with P3400 on is an error, since P3400 subsumes
  P4451; `-fcontracts`, unless given explicitly, is on when any per-paper
  option is.  The `contract-options-additive-*.C` tests run every
  combination of the three P4451 options and their `-fno-` forms in C++23,
  in both orders where the old rule depended on it, and the driver's link
  of the contracts runtime.

  A per-paper option left on when contracts end up off -- which only a
  final explicit `-fno-contracts` does -- is an error naming the option
  given, not the ones it implies; a `-fno-` form is always accepted.  C has
  the same rule one level down: `-fcontracts-p3100`, `-fcontracts-p4298`
  and `-fcontracts-p4405` turn `-fcontracts-p4299` on, are errors with a
  final `-fno-contracts-p4299`, and the C driver links the contracts
  runtime for each.  So does the umbrella `-fcontracts-p3850`, which in C
  implies those three and turns C contracts on even with all three
  disabled.  The contracts options that mean nothing with contracts
  off, and the P4405 strategies given positively with P4405 off, warn that
  they have no effect -- a `-fno-` form never does, a joined option does
  for any value, its default included; `-fcontract-violation-information=`
  does not, as `<contracts.h>` and `<cassert>` honour it whatever the
  contracts options.
  The `contract-options-rule-*.C` and `c-contract-options-*.c` tests cover
  each case, in both orders where order could matter.

* **The evaluation-semantic spelling.**  `-fcontract-evaluation-semantic=`
  has one help line in `c.opt` and one synopsis line in `invoke.texi`, each
  of which spells out all seven semantics -- so each names P3100 (`assume`)
  and P4298 (`noexcept_enforce` / `noexcept_observe`) in a single string.
  The `EnumValue` rows behind those names, and the widening of the option
  record to C, did divide and are with P3100, P4298 and P4299 respectively.

* **The `invoke.texi` synopsis lines.**  Three lines of four, four and
  three `-fcontracts-pNNNN` flags, and in the Warning Options synopsis one
  that pairs `-Wno-contract-configuration` (P3595) with
  `-Wno-contract-invalid-label-facet` (P3400).  `@gccoptlist` renders a
  run-on list; the line breaks are arbitrary, but a line is still the unit a
  cut can move.  The `@opindex` and `@item` entries those flags index are one
  per line and are with their papers.

* **The `invoke.texi` roll-call.**  The description shared by the per-paper
  options: a table naming each paper and the feature-test macro it defines,
  and a note that P3100's and P4298's options apply to C too.  It is one
  table, written once for every paper.

* **The `invoke.texi` paragraph on running a contracts program.**  A
  program linked with contracts enabled -- by `-std=c++26`, `-fcontracts`,
  any per-paper option, or in C `-fcontracts-p4299` or an option that
  enables it -- depends on the shared `libcontracts.so.1`, so an
  installation outside the system's library path needs `-Wl,-rpath` or
  `LD_LIBRARY_PATH` to run it, as for its `libstdc++.so.6`.  The paragraph
  names the options of every paper at once.

## Compile gap

None: every option, flag and warning this commit names is declared by a
commit in the list above, and the `.opt` and `.texi` fragments it completes
are syntactically valid without it.

What it does carry is a *behaviour* gap, and it is the whole branch's, not
this commit's alone.  **Until this commit lands, no `-fcontracts-pNNNN`
implies `-fcontracts`.**  Each per-paper option sets only its own
`flag_contracts_pNNNN`; nothing turns the base facility on.  So from
`2900-umbrella-option` all the way to here, every paper's own tests -- which
compile with `-fcontracts-pNNNN` and expect contracts to be enabled -- fail.
The papers are individually correct and individually reviewable throughout;
the branch is only testable as a whole from this commit onward.  Each
paper's `## Compile gap` records the same thing from its own side.

## Contents

- gcc/c-family/c-opts.cc : /Contracts are in C\+\+26/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-*.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-*.C : *
- gcc/testsuite/gcc.dg/contracts/c-contract-options-*.c : *
- gcc/testsuite/gcc.dg/asan/p3100-c-routing-implied-p4299.c : *
- gcc/c-family/c.opt : /fcontracts-p3097 \|\| fcontracts-p3098/, /quick_enforce\|assume\|noexcept_enforce/
- gcc/doc/invoke.texi : #7:1, /quick_enforce@r\{\|\}assume/, /^-fcontracts-p3097  -fcontracts-p3098  -fcontracts-p3099  -fcontracts-p3100$/, /^Enable an individual proposed extension/, /^-Wno-contract-configuration  -Wno-contract-invalid-label-facet$/, /^C and Objective-C too \(@pxref\{C Dialect Options\}\)\.$/, /^These options add up, and their @option\{-fno-\} forms are always accepted;$/, /^giving one of them positively with a final @option\{-fno-contracts\} is an$/, /^error, as is @option\{-fno-contracts-p4451\} with @option\{-fcontracts-p3400\}$/, /^in effect\.  The other contracts options have no effect when contracts are$/, /^and GCC warns when one of them is given then, except in its negative$/, /^@option\{-fno-\} form\.  In C, the per-paper options that apply to C relate$/, /^to @option\{-fcontracts-p4299\} in the same way\.$/, /^takes effect whatever the contracts options, and GCC never warns for it\.$/
