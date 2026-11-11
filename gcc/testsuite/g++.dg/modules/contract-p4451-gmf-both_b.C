// Importer of contract-p4451-gmf-both_a.C: the declaration in that module's
// global module fragment does not precede the assertion-control-specifier.
// { dg-additional-options "-fmodules -fcontracts-p3400 -fcontracts-p4451" }

import p4451g_both;

void h (int x) pre<terminating> (x > 0);	// { dg-error "'terminating' was not declared in this scope" }
