// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// The inline configuration option, like the other configuration options,
// warns that it has no effect when an explicit -fno-contracts turns
// contracts off (contract-options-rule-no-effect-no-contracts.C); the
// configuration is not parsed.

// { dg-do compile { target c++26 } }
// { dg-additional-options {-fno-contracts -fcontract-configuration=\[\]} }
// { dg-warning "contract configuration options have no effect without .-fcontracts." "" { target *-*-* } 0 }

int i;
