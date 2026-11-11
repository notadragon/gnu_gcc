// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// Every per-paper option given positively is diagnosed with a final
// -fno-contracts, each once, including -fcontracts-p3100 (which would
// otherwise route the sanitizers) and the -fcontracts-p3850 umbrella.

// { dg-do compile }
// { dg-additional-options "-std=c++23 -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p3100 -fcontracts-p3290 -fcontracts-p3400 -fcontracts-p3850 -fcontracts-p4283 -fcontracts-p4298 -fcontracts-p4301 -fcontracts-p4405 -fcontracts-p4451 -fno-contracts" }
// { dg-error ".-fcontracts-p3097. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3098. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3099. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3100. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3290. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3400. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p3850. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p4283. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p4298. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p4301. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p4405. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }
// { dg-error ".-fcontracts-p4451. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }

int i;
