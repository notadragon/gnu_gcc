---
id: 0430-refinable-body-contracts
subject: 'c++, contracts: Mark contract-holding functions as refinable bodies; remove -fcontracts-conservative-ipa'
depends: [0420-refinable-body]
regenerates: []
fixes: [gcc-689, gcc-896]
---

## Rationale

The evaluation semantic of a contract assertion is chosen per evaluation
and can differ between translation units ([basic.contract.eval]), also
between the definitions of one inline function, function template
instantiation or other vague-linkage function; mixing them is not an ODR
violation.  A caller that does not inline such a function and relies on
facts derived from its own unit's definition may run another unit's
definition, and the program then does something no permitted choice of
semantics allows.  Both directions happen: a check whose failure path does
not return (an enforced check calling std::terminate) gives the function a
return value range, nonnull, noreturn or mod-ref facts that the definition
which ignores the assertion does not honour; and a definition whose
assertion is ignored has no call, so it is found nothrow (by the front end
and by the middle end), const or store-free, while another definition calls
a violation handler that may throw or write memory -- a caller's catch is
then skipped (std::terminate) and its locals are not destroyed.
PR121936 discusses the general problem.

The front end now marks every function definition that holds a contract
assertion with the middle end's refinable-body mark
(`mark_refinable_body`): `cp_genericize_r` marks the function whose body
holds a contract check (an assertion, or a precondition or postcondition
evaluated in the body or in an outlined check function) and the function
that calls a contract wrapper (a client-side check of the callee's
contracts: the wrapper is internal, and the caller's definition in another
unit may check them differently or not at all), and
`finish_function` marks a function with precondition or postcondition
specifiers whatever this unit's semantic, since an ignored specifier
leaves nothing in the body.  This happens before cloning, so constructor
and destructor clones inherit the mark, and it covers template
instantiations and lambdas, which are finished the same way.  Such a body
may still be inlined, cloned and split, and the code after a check still
relies on its predicate inside the function; only calls that do not
inline a function that may bind to another definition stop using facts
from this body.  `finish_function` no longer sets `TREE_NOTHROW` from the
body of a marked function (it used `decl_replaceable_p`, which is false
for COMDAT definitions); a static or strong external definition, the only
definition of its symbol, still gets nothrow from the middle end.

That replaces `-fcontracts-conservative-ipa` (on by default), which
called std::terminate through an internal `noipa` wrapper,
`__tu_terminate_wrapper`, so that nothing after a failed check was known
not to return.  It hid only one direction, also took the predicate away
from the code after the check in the function itself, and made a
quick_enforce check whose predicate may throw draw a bogus
`-Wmaybe-uninitialized` warning about the check's temporary (the reaction
appeared to return).  The option and the wrapper are removed: enforced
and observed checks call `__tu_has_violation`, which now calls
std::terminate directly, and quick_enforce checks call std::terminate.
That fixes GCC-689: the reaction does not return, so the temporary is no
longer read unset (contract-quick-enforce-throwing-predicate-uninit.C).

Tests: g++.dg/contracts/cpp26/refinable-body-1.C (ignored assertions:
callers of an inline function, a template instantiation, a constructor, a
member function and a lambda keep their landing pads, a static and an
external function's callers lose them, as do an inline function's without
a contract assertion), refinable-body-2.C (enforced
assertions: the caller of an inline function keeps its comparison, the
static and external controls fold it, the comparison after the check in
the function folds, no wrapper symbol), and the multi-unit run tests
g++.dg/lto/contracts-refinable-1 to -4, which compile the units with
different semantics (per-file options need the LTO harness) and run
without and with `-flto`, in both link orders: a caller in an enforcing
unit linked with an ignoring definition, and a catch and a destructor in
an ignoring unit linked with a definition whose handler throws;
contracts-refinable-5 links a caller compiled with an enforced client
check of the callee's postcondition, kept out of line (`-fno-inline`), with
a definition of that caller that checks nothing.

## Compile gap

None.

## Contents

- gcc/c-family/c.opt : #r0
- gcc/c-family/c.opt.urls : #r0
- gcc/cp/contracts.cc : #r2, #r3, #r4, #r5
- gcc/cp/cp-gimplify.cc : #6:0, #6:2, /mark_refinable_body/, /mark_contract_wrapper_caller \(fn\);/
- gcc/cp/decl.cc : #18:0, #18:2, /refinable/
- gcc/doc/invoke.texi : #r0, #r1, #1:2
- gcc/testsuite/g++.dg/contracts/cpp26/contract-quick-enforce-throwing-predicate-uninit.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-1.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-2.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-1_0.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-1_1.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-2_0.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-2_1.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-3_0.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-3_1.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-4_0.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-4_1.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-5_0.C : *
- gcc/testsuite/g++.dg/lto/contracts-refinable-5_1.C : *
