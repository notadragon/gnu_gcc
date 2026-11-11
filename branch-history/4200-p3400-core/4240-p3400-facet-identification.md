---
id: 4240-p3400-facet-identification
subject: 'c++: contracts: P3400: the identification (group_names) label facet'
depends:
  - 4200-p3400-core
regenerates: []
fixes: []
---

## Rationale

`identification_label`: the facet that associates a contract assertion with
one or more named groups, so that a configuration file can select semantics
by group rather than by source location.

The whole of the front-end side is `ensure_contract_groups`: probe the
label for a const, accessible `group_names`, read its names, and keep them
as a `TREE_LIST` of `STRING_CST` group names hung off the contract.
[support.contract.control.ident] makes `group_names` any const range whose
elements convert to `string_view`, so the names are read the way a
range-based `for` walks a range: `build_group_names_query` builds an
expression that finds `begin` and `end` (as members, or by
argument-dependent lookup), advances the iterator, converts the element to
`std::string_view` -- or to `const char *` when `<string_view>` has not
declared it, which is all an element can convert to then -- and yields the
number of elements, an array of the names' lengths, or an array of all
their characters.  `eval_group_names_query` constant-evaluates it, three
times in all, and each evaluation walks the range once, so reading costs
time linear in the size of the names, whatever the iterator.  Each name is
read within the full-expression that dereferences the iterator, so an
iterator may return its names by value.  Nothing depends on the range's
type saying how many names it holds, so a `span` or an `initializer_list`
is read in full.  Empty names are skipped.

A label holds at most 4096 names (`GROUP_NAMES_MAX`): the count stops one
past it, before any name is read, and a range or array of more is an
error, "the 'group_names' facet ... holds more than 4096 names".  A
silent cut-off would configure the label as in fewer groups than it
names; the cap keeps a pathological range from costing unbounded
translation time.

A `group_names` that is not such a range -- the query does not form --
gives the label no groups, undiagnosed, exactly as an absent one does.
One that is such a range but whose reading is not a constant expression
(undefined behaviour in a user `begin`, a name that is not a constant, the
constexpr operation limit) is an error, "the 'group_names' facet ... must
be usable in a constant expression", as for the other facets consumed
during translation, followed by the evaluator's own account of why.
Giving such a label no groups would silently configure its assertions as
unlabelled.  A label still dependent is not read: it is read again once
instantiated.

The names are read once per contract, when its label is resolved
(`resolve_contract_label`), and cached in `CONTRACT_GROUPS`, with
`error_mark_node` as the "this label has no groups" answer.  Every copy of
the contract made afterwards -- the body saved for constant evaluation, a
caller-side check -- carries the result, so a label whose reading fails is
diagnosed once, at the label, and the evaluator's reasons are given there
too.  In a template the names are read when the contract is instantiated.

A `string_view` name is the whole view, NUL bytes included
(notadragon_wg21 DECISIONS.md E191 (a)), and is matched on all of its bytes
(E192 (2)); a `const char*` name ends at its NUL.

`p3400-identification-label-range.C` and its `-run` and `-no-header`
companions cover the range forms -- arrays of pointers with short and long
names, a `std::array` and a C array of `string_view`, an
`initializer_list`, a `span`, ranges with member and with ADL `begin` and
`end` -- and combined labels, during constant evaluation and with a
configuration file selecting by group at run time.
`p3400-combined-label-nesting.C` reads labels combined sixteen levels deep,
which stay within the constexpr operation limit only because the library's
union asks a nested union for its next name rather than iterating it.
`p3400-group-names-not-constant.C` (undefined behaviour in `begin`, a
name that is not a constant, and two wrongly shaped `group_names` that
stay silent) and `p3400-group-names-ops-limit.C` (a lowered
`-fconstexpr-ops-limit`) pin the error.  `p3400-group-names-read-in-full.C`
pins that names are read in full: one longer than 256 characters, one
holding the bytes 0xFE or 0xFF or a newline, and ranges of 255 and more
names each select exactly their own groups.  `p3400-group-names-too-many.C` and
its `-string-view` companion pin the cap (a range, an array and a
class-iterator range of 4097 names are errors; 4096 are read in full, through
pointers, an array and a class-type iterator, within a timeout; names
returned by value are read).  `p3400-group-names-reported-once.C` pins one
report per label, at the label, when the assertion is constant-evaluated,
for a name that is not a constant and for too many names.
`p3400-group-names-string-view-nul.C` pins that a `string_view` name holding
a NUL is not the name before it, and is within a configured group it starts
with followed by a dot -- also a group that is the name up to its NUL, and a
leaf of a combined group -- while a `const char*` name ends at its NUL; its
`-run` companion pins the same for the semantic of a run-time check.
`p3400-group-names-bytes.C` pins that the bytes 0, newline and 1 in a
name are kept apart from each other and from the other bytes.
`p3400-facet-group-const.C` pins
the const requirement.  P3400 wants
`group_names` const so that nothing suggests a label's group membership
could change at run time and have an effect: nothing reads it after
translation.

## Compile gap

None: `label_facet_accessible_p`, `CONTRACT_GROUPS` and the lazy-resolution
call site are all in `4200-p3400-core`.

The trap is the memoized sentinel.  With the framework present and this
facet absent, `CONTRACT_GROUPS` is simply never populated, and every
consumer treats that exactly as it treats a label with no `group_names` at
all: the assertion compiles, the configuration file's group entries match
nothing, and the violation is reported with an empty group list.  No
diagnostic is possible, because "not a member of any group" is a legal and
common state.  A group-keyed configuration therefore silently degrades to
the default semantic for every assertion -- which is why
`p3400-group-basic.C` and friends assert on the semantic that actually ran
rather than on a diagnostic.

## Contents

- gcc/cp/contracts.cc : @ensure_contract_groups, @resolve_contract_label:1, @resolve_contract_label:5, /get_identifier \("group_names"\)/, /^#define GROUP_NAMES_MAX 4096$/, /^enum group_names_query/, /^enum group_names_read/, @build_group_names_query, @group_names_array_values, /^eval_group_names_query \(location_t loc, tree gn, group_names_query what,$/
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-combined-label-nesting.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-group-const.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-basic.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-combined.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-char-array-forms.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-not-constant.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-ops-limit.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-read-in-full.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-reported-once.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-string-view-nul.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-string-view-nul-run.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-bytes.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-bytes.json : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-too-many.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-names-too-many-string-view.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-multi.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-postcondition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-with-facets.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range-no-header.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range-run.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-identification-label-range-run.json : *
- libstdc++-v3/testsuite/18_support/contracts/labels_identification_range.cc : *
