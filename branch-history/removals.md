# Declared removals

Upstream lines the branch deletes, declared so that deleting them does not
re-align the diff around them.  Format and rules:
`notadragon_wg21/src/tools/branchhistory/removals.py`.  Each item becomes one
segment, keyed `<path>#rN` (N counts from 0 under its heading) and claimed
like any other.

## gcc/cp/contracts.cc

- -/^contract_evaluation_semantic$/ through -/^\{$/ before +/^\/\* Read the cached constexpr callee-side semantic\./
- -/^  \/\* Try to get the actual source text for the condition; if that fails pretty$/ through -/^\t+ +condition\.get_finish \(\)\);$/ before +/^  \/\* -fcontract-comment-from-expression: use the pretty-printed form even$/

## gcc/cp/contracts.h

- -/^extern contract_evaluation_semantic get_evaluation_semantic \(const_tree\);$/ before +/^extern contract_evaluation_semantic get_constexpr_evaluation_semantic$/
- -/^\/\* Will this contract be ignored\.  \*\/$/ until -/^#endif \/\* ! GCC_CP_CONTRACT_H \*\/$/ before +/^\/\* Contract data-block ABI constants\.$/
