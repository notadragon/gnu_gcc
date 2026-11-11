// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize= has no -fno- form.
// { dg-do compile }
// { dg-options "-fno-contracts-prove-optimize=2" }
// { dg-error "unrecognized command-line option '-fno-contracts-prove-optimize=2'" "" { target *-*-* } 0 }

int i;
