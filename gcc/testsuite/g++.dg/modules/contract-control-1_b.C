// Consumer for contract-control-1_a.C.  The short name must resolve inside
// an assertion-control expression, and must NOT leak into ordinary lookup
// -- the whole point of the separate vector.

// { dg-additional-options "-fmodules -fcontracts -fcontracts-p3400" }

import g9lbl;

void
f (int x) pre<my_label> (x > 0)
{
}

// Still not an ordinary name.
auto leak = my_label;  // { dg-error "'my_label' was not declared in this scope" }
