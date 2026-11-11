// Importer of contract-p4451-impl_a.C, whose import of std is not
// exported: no declaration of std::contracts::labels::terminating precedes
// the assertion-control-specifier.
// { dg-additional-options "-fmodules -fcontracts-p4451" }

import p4451m;

void h (int x) pre<terminating> (x > 0);	// { dg-error "no declaration of .std::contracts::labels::terminating. precedes" }
