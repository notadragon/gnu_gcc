// P3595: contract configuration diagnostics are reported against the
// configuration source (file:line:column), not against the first contract
// assertion that happens to be parsed.  The configuration is parsed and
// validated eagerly, so these diagnostics fire even though this translation
// unit contains no contract assertions at all.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-errors.json" }
//
// The diagnostics point into the JSON file rather than this source, so match
// them with dg-regexp.  Entry 0 has an unknown key (warning, entry line 2);
// entry 1 has an invalid semantic (error, entry line 5).  Each diagnostic is
// preceded by a JSON-pointer path line identifying the offending value.
// { dg-regexp {[^\n]*p3595-config-errors\.json: In JSON value '/0/match/wibble'} }
// { dg-regexp {[^\n]*p3595-config-errors\.json:2:[0-9]+: warning: unknown key 'wibble' in 'match' object[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-errors\.json: In JSON value '/1/output/semantic'} }
// { dg-regexp {[^\n]*p3595-config-errors\.json:5:[0-9]+: error: invalid contract evaluation semantic 'bogus'[^\n]*} }

int f (int x) { return x; }
