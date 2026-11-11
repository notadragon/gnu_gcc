// DECISIONS.md E175-E177 (GCC-964): the contracts option rule.
// With -fno-contracts, only the options actually given are diagnosed:
// -fcontracts-p3850, not each per-paper option it implies, nor
// -fcontracts-p4451, which -fcontracts-p3850 and -fcontracts-p3400 imply.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fno-contracts -fcontracts-p3850" }
// { dg-error ".-fcontracts-p3850. cannot be combined with .-fno-contracts." "" { target *-*-* } 0 }

int i;
