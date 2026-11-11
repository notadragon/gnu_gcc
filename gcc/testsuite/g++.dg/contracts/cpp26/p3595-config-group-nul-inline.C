// A "group" in an inline P3595 configuration entry holding a NUL character
// is an error, not cut at the NUL (notadragon_wg21 DECISIONS.md E192
// (1)(a)).  p3595-config-group-nul.C is the file form.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"group":"safety\\u0000x"},"output":{"semantic":"ignore"}}\]} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/group'\n<command-line>:1:[0-9]+: error: 'group' must not contain a null character[^\n]*} }

int f (int x) pre (x > 0) { return x; }
