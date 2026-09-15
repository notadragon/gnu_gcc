---
id: 2000-refactoring
subject: 'c++: contracts: behaviour-neutral restructuring'
depends: []
regenerates: []
fixes: []
---

## Rationale

The QoI band leads with the one commit that carries no behaviour at all, so
that a reviewer's first stop on that band is a no-op and everything after it
is a real change.  What is here is the residue of the contract-parser rework
that could be pulled out cleanly: six stale or redundant comments deleted
from `cp_parser_function_contract_specifier' and
`cp_parser_late_contract_condition' (each its own one-line hunk in the net
diff), and a pair of braces added around the single statement of an `if' in
`tsubst_pack_index'.  None of them belongs to a feature, and none of them is
worth a commit of its own.  Its sibling, `2600-refactoring-after-libcontracts`,
holds the restructuring that could not lead in the same way because it
depends on libcontracts existing first.

## Compile gap

None.  Every change here is either the deletion of a comment or the addition
of braces around an already-single-statement `if' body; nothing is declared,
renamed, or given new behaviour, so nothing outside this commit changes
shape.

## Contents

- gcc/cp/parser.cc : /Parse the condition, ensuring that parameters or the return variable/, /Skip until we reach a closing token \)\./, /Build a deferred-parse node\./, /And its corresponding contract\./, /If we have a current class object, see if we need to consider/, /Build a fake variable for the result identifier/, /Revert \(any\) constification of the current class object/
- gcc/cp/pt.cc : @tsubst_pack_index

