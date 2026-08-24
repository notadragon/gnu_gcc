// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-caller-parse-good.json" }
// Parsing of the "caller" object form: valid keys accepted, module/function warn.
// The warning is reported against the configuration file, preceded by a
// JSON-pointer path line identifying the offending value.
int f(int x) pre(x > 0) { return x; }
int main() { return f(1); }
// { dg-warning "unknown key .module." "" { target *-*-* } 0 }
// { dg-regexp {[^\n]*p3595-caller-parse-good\.json: In JSON value '/0/match/caller/module'} }
