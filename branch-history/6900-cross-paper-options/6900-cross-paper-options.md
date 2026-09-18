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
  - 5000-p4299
  - 5200-p4301
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

* **The evaluation-semantic spelling.**  `-fcontract-evaluation-semantic=`
  has one help line in `c.opt` and one synopsis line in `invoke.texi`, each
  of which spells out all seven semantics -- so each names P3100 (`assume`)
  and P4298 (`noexcept_enforce` / `noexcept_observe`) in a single string.
  The `EnumValue` rows behind those names, and the widening of the option
  record to C, did divide and are with P3100, P4298 and P4299 respectively.

* **The `invoke.texi` synopsis block.**  Three lines of four, four and three
  `-fcontracts-pNNNN` flags, plus two more that pair
  `-fcontracts-allow-assume` with `-Wcontract-configuration` and
  `-Wcontract-constexpr-side-effect` with `-Wcontract-invalid-label-facet`.
  `@gccoptlist` renders a run-on list; the line breaks are arbitrary, but a
  line is still the unit a cut can move.  The `@opindex` and `@item` entries
  those flags index are one per line and are with their papers.

* **The `invoke.texi` roll-call prose.**  One sentence -- "contracts on
  virtual functions (P3097), postcondition captures (P3098), ..." -- naming
  every paper.  Splitting a sentence across eleven commits would produce
  eleven ungrammatical fragments.

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
- gcc/c-family/c.opt : /fcontracts-p3097 \|\| fcontracts-p3098/, /quick_enforce\|assume\|noexcept_enforce/
- gcc/doc/invoke.texi : /quick_enforce@r\{\|\}assume/, /^-fcontracts-p3850  -fcontracts-p3097  -fcontracts-p3098  -fcontracts-p3099$/, /^Enable an individual proposed extension/, /^undefined behaviour \(P3100\), integration with @code\{assert\} \(P3290\),$/, /^assertion-control labels \(P3400\), @code\{requires\} clauses on contract$/
