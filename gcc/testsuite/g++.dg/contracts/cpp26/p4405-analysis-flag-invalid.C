// D4405: the analysis evaluation's semantic is one of ignore, diagnose and
// prove; -fcontract-analysis-semantic= rejects any other.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=enforce" }
// { dg-error "the analysis evaluation semantic must be 'ignore', 'diagnose' or 'prove', not 'enforce'" "" { target *-*-* } 0 }

int f (int x) pre (x > 0) { return x; }
