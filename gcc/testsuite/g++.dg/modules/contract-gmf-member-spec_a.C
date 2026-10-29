// An explicit specialization of a member template, with a precondition,
// defined in a global module fragment and exported (the PR c++/127787 shape,
// gmf-8_a.C, with a contract).  contract-gmf-member-spec_b.C imports it and
// then includes the same header again, which P1811 lets redefine it.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fmodules -fcontracts -fcontract-evaluation-semantic=observe" }

module;
#include "contract-gmf-member-spec.h"
export module contract_gmf_member_spec;
// { dg-module-cmi "contract_gmf_member_spec" }
export using N::A;
