// Importer of contract-p4451-gmf-p3400_a.C: the declaration in that module's
// global module fragment does not precede the assertion-control-specifier.
// { dg-additional-options "-fmodules -fcontracts-p3400" }

import p4451g_p3400;

void h (int x) pre<terminating> (x > 0);	// { dg-error "'terminating' was not declared in this scope" }
