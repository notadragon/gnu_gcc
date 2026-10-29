// An exported inline function and an exported non-inline function with a
// checked contract: writing the CMI streams a body that calls a
// __cxa_contract_violation_* entry point, which must therefore have a
// DECL_CONTEXT.  contract-inline-checked_b.C imports and runs them.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts -fcontract-evaluation-semantic=observe" }

export module contract_inline_checked;
// { dg-module-cmi "contract_inline_checked" }

export inline int ni (int x) pre (x > 0) { return x; }
export int nn (int x) pre (x > 0) { return x; }
