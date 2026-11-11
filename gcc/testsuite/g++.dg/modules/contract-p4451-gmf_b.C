// Importer of contract-p4451-gmf_a.C: the declaration in that module's
// global module fragment does not precede the assertion-control-specifier.
// { dg-additional-options "-fmodules -fcontracts-p4451" }

import p4451g;

void h (int x) pre<terminating> (x > 0);	// { dg-error "no declaration of .std::contracts::labels::terminating. precedes" }
