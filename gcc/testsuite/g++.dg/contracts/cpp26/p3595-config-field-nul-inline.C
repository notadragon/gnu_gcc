// A NUL character in any string field of an inline P3595 configuration
// entry is an error, not cut at the NUL (notadragon_wg21 DECISIONS.md E193
// (1)): each -fcontract-configuration= below holds one in a different
// field, and each is diagnosed.  p3595-config-field-nul.C is the file form.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"kind":"pre\\u0000x"},"output":{"semantic":"ignore"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"group":"g\\u0000x"},"output":{"semantic":"ignore"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"namespace":"n\\u0000x"},"output":{"semantic":"ignore"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"location":"t.C\\u0000x"},"output":{"semantic":"ignore"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"caller":{"location":"t.C\\u0000x"}},"output":{"semantic":"ignore"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"match":{"caller":{"namespace":"n\\u0000x"}},"output":{"semantic":"ignore"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"output":{"semantic":"ignore\\u0000x"}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"output":{"semantic":"ignore","dynamic":{"name":"sel\\u0000x"}}}\]} }
// { dg-additional-options {-fcontract-configuration=\[{"output":{"semantic":"ignore","dynamic":{"name":"sel","linkage":"C\\u0000x"}}}\]} }
//
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/kind'\n<command-line>:1:[0-9]+: error: 'kind' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/group'\n<command-line>:1:[0-9]+: error: 'group' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/namespace'\n<command-line>:1:[0-9]+: error: 'namespace' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/location'\n<command-line>:1:[0-9]+: error: 'location' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/caller/location'\n<command-line>:1:[0-9]+: error: 'location' in 'caller' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/match/caller/namespace'\n<command-line>:1:[0-9]+: error: 'namespace' in 'caller' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/output/semantic'\n<command-line>:1:[0-9]+: error: 'semantic' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/output/dynamic/name'\n<command-line>:1:[0-9]+: error: 'name' in 'dynamic' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*<command-line>: In JSON value '/0/output/dynamic/linkage'\n<command-line>:1:[0-9]+: error: 'linkage' must not contain a null character[^\n]*} }

int f (int x) pre (x > 0) { return x; }
