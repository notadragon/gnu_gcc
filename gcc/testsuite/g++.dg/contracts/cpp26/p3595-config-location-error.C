// P3595: a malformed line-range list in a "location" match criterion is
// diagnosed against the configuration source, rather than sending the parser
// into an infinite loop.  Each of the three entries below carries a different
// malformed range ("10x" -- trailing junk; "1-2z" -- trailing junk after a
// range; "5-" -- a range with no upper bound), and each is reported and then
// skipped.  The configuration is validated eagerly, so these fire even though
// this translation unit contains no contract assertions at all.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-location-error.json" }
//
// The diagnostics point into the JSON file, so match them with dg-regexp.
// Each error is preceded by a JSON-pointer path line identifying the value.
// { dg-regexp {[^\n]*p3595-config-location-error\.json: In JSON value '/0/match/location'} }
// { dg-regexp {[^\n]*p3595-config-location-error\.json:[0-9]+:[0-9]+: error: malformed line range 'err.cpp:10x' in 'location'[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-location-error\.json: In JSON value '/1/match/location'} }
// { dg-regexp {[^\n]*p3595-config-location-error\.json:[0-9]+:[0-9]+: error: malformed line range 'err.cpp:1-2z' in 'location'[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-location-error\.json: In JSON value '/2/match/location'} }
// { dg-regexp {[^\n]*p3595-config-location-error\.json:[0-9]+:[0-9]+: error: malformed line range 'err.cpp:5-' in 'location'[^\n]*} }

int f (int x) { return x; }
