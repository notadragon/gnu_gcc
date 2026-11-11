// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// The configuration-file option, like the other configuration options,
// warns that it has no effect when an explicit -fno-contracts turns
// contracts off (contract-options-rule-no-effect-no-contracts.C); the file,
// which does not exist, is not read.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts -fcontract-configuration-file=no-such-file.json" }
// { dg-warning "contract configuration options have no effect without .-fcontracts." "" { target *-*-* } 0 }

int i;
