// Consumer for contract-control-2_a.H.  The short name resolves inside an
// assertion-control expression and does not leak into ordinary lookup.

// { dg-additional-options "-fmodules -fcontracts -fcontracts-p3400" }

import "contract-control-2_a.H";

void
f (int x) pre<my_label> (x > 0)
{
}

// Still not an ordinary name.
auto leak = my_label;  // { dg-error "'my_label' was not declared in this scope" }
