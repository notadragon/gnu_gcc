---
id: 1260-outlined-lambda-capture-proxy
subject: "c++: contracts: P2900: remap a lambda's capture proxies into outlined checks"
depends: []
regenerates: []
fixes: [gcc-610]
---

## Rationale

GCC-610.  Under -fcontract-checks-outlined a contract on a lambda that names
one of the lambda's explicit captures crashed the compiler (in
gimplify_var_or_parm_decl).  The capture is named through a capture proxy, a
local VAR_DECL whose DECL_VALUE_EXPR reads the capture through the closure
parameter.  remap_contract copied the proxy along with the rest of the
predicate, and the copy kept its value expression, still naming the
parameter of the lambda's operator() rather than that of the outlined
function.

Each proxy in the predicate is now mapped to its value expression, remapped
like the rest of the predicate, so the outlined function reads the capture
through its own closure parameter.

The outlined check functions of a lambda in a function template are
internal and in the comdat group of the lambda's operator().
`start_preparsed_function` gives a function of a local class in a template
comdat linkage of its own, which left them in two groups and failed
`verify_cgraph_node` on a checking build; it now leaves a contract check
function's linkage alone (GCC-657, `contract-outlined-lambda-in-template.C`).

## Compile gap

None: a self-contained addition to remap_contract.

## Contents

- gcc/cp/contracts.cc : /A lambda's capture proxy is a local whose DECL_VALUE_EXPR reads the/, /walk_tree \(&TREE_VALUE \(cap\), copy_tree_body_r, &id, NULL\);/
- gcc/cp/decl.cc : /An outlined contract check function already has its linkage:/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-lambda-capture-outlined-run.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-lambda-outlined.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-outlined-lambda-in-template.C : *
