# Declared removals

Upstream lines the branch deletes, declared so that deleting them does not
re-align the diff around them.  Format and rules:
`notadragon_wg21/src/tools/branchhistory/removals.py`.  Each item becomes one
segment, keyed `<path>#rN` (N counts from 0 under its heading) and claimed
like any other.

## gcc/cp/contracts.cc

- -/^contract_evaluation_semantic$/ through -/^\{$/ before +/^\/\* Read the cached constexpr callee-side semantic\./
- -/^  \/\* Try to get the actual source text for the condition; if that fails pretty$/ through -/^\t+ +condition\.get_finish \(\)\);$/ before +/^  \/\* -fcontract-comment-from-expression: use the pretty-printed form even$/
- -/^static GTY\(\(\)\) tree tu_terminate_wrapper = NULL_TREE;$/ until -/^\/\* Lookup a name in std::contracts, or inject it\.  \*\/$/ before +/^\/\* P2900 quick_enforce: terminate the program via __builtin_trap \(\) -- no$/
- -/^  \/\* We might need the terminate wrapper, even if we do not use the violation$/ through -/^    build_terminate_wrapper \(\);$/ before +/^  emit_pending_weak_selectors \(\);$/
- -/^  if \(flag_contracts_conservative_ipa\)$/#1 through -/^    terminate_wrapper = tu_terminate_wrapper;$/ before +/^  emit_pending_weak_selectors \(\);$/
- -/^  if \(flag_contracts_conservative_ipa\)$/#2 through -/^    terminate_wrapper = declare_terminate_wrapper \(\);$/ before +/^  \/\* Unshare the condition: this helper may be called several times for the$/

## gcc/cp/contracts.h

- -/^extern contract_evaluation_semantic get_evaluation_semantic \(const_tree\);$/ before +/^extern contract_evaluation_semantic get_constexpr_evaluation_semantic$/
- -/^\/\* Will this contract be ignored\.  \*\/$/ until -/^#endif \/\* ! GCC_CP_CONTRACT_H \*\/$/ before +/^\/\* Contract data-block ABI constants\.$/

## gcc/c-family/c.opt

- -/^fcontracts-conservative-ipa$/ until -/^fcontract-checks-outlined$/ before +/^fcontract-checks-outlined$/

## gcc/c-family/c.opt.urls

- -/^fcontracts-conservative-ipa$/ until -/^fcontract-checks-outlined$/ before +/^fcontract-checks-outlined$/

## gcc/doc/invoke.texi

- -/^-fcontracts-conservative-ipa  -fcontract-checks-outlined$/ after +/^-fcontract-checks-outlined$/
- -/^@opindex fcontracts-conservative-ipa$/ until -/^@opindex fcontract-checks-outlined$/ before +/^@opindex fcontract-checks-outlined$/
