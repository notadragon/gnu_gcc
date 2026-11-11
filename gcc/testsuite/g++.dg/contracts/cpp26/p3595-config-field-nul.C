// A NUL character in any string field of a P3595 configuration entry is an
// error, not cut at the NUL (notadragon_wg21 DECISIONS.md E193 (1)): each
// entry of p3595-config-field-nul.json holds one in a different field,
// and each is diagnosed and skipped.  p3595-config-field-nul-inline.C is
// the inline form; p3595-config-group-nul.C is the "group" field alone.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-field-nul.json" }
//
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/0/match/kind'\n[^\n]*p3595-config-field-nul\.json:2:[0-9]+: error: 'kind' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/1/match/group'\n[^\n]*p3595-config-field-nul\.json:3:[0-9]+: error: 'group' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/2/match/namespace'\n[^\n]*p3595-config-field-nul\.json:4:[0-9]+: error: 'namespace' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/3/match/location'\n[^\n]*p3595-config-field-nul\.json:5:[0-9]+: error: 'location' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/4/match/caller/location'\n[^\n]*p3595-config-field-nul\.json:6:[0-9]+: error: 'location' in 'caller' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/5/match/caller/namespace'\n[^\n]*p3595-config-field-nul\.json:7:[0-9]+: error: 'namespace' in 'caller' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/6/output/semantic'\n[^\n]*p3595-config-field-nul\.json:8:[0-9]+: error: 'semantic' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/7/output/dynamic/name'\n[^\n]*p3595-config-field-nul\.json:9:[0-9]+: error: 'name' in 'dynamic' must not contain a null character[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-field-nul\.json: In JSON value '/8/output/dynamic/linkage'\n[^\n]*p3595-config-field-nul\.json:10:[0-9]+: error: 'linkage' must not contain a null character[^\n]*} }

int f (int x) pre (x > 0) { return x; }
