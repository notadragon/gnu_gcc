// P3595 x config: a known key present with the wrong JSON type is diagnosed
// and its entry skipped, rather than being silently ignored.
//
// Each of these is a hard error, not a warning, and the offending entry is
// dropped rather than pushed with the bad key ignored.  Both halves matter:
// dropping a *match* criterion would make an entry LESS selective, so a
// mistyped "kind" that was merely ignored would turn a narrowly-scoped
// entry into a catch-all changing the semantic of every contract in the
// translation unit.  Because the diagnostic is an error the translation
// unit does not compile at all, so that mis-selection can never reach
// generated code -- which is why this test observes the diagnostics and
// not the resulting behaviour.
//
// Clang independently caught up to this same validation (its own A4); mirror
// the test to GCC too, since GCC had the behavior but no regression test for
// it.  Configuration is validated eagerly, so these fire even though this
// translation unit contains no contract assertions.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-wrong-type.json" }
//
// The diagnostics point into the JSON file, so match them with dg-regexp.
// Each error is preceded by a JSON-pointer path line identifying the value.
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/0/match/kind'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'kind' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/1/match/group'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'group' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/2/match/namespace'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'namespace' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON array '/3/match/location'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'location' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/4/match/constexpr'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'constexpr' must be a boolean[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/5/match/caller/location'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'location' in 'caller' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/6/match/caller/namespace'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'namespace' in 'caller' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/7/output/semantic'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'semantic' must be a string[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON object '/8/output/dynamic'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'dynamic' requires a string 'name'[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/9/output/dynamic/linkage'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'linkage' must be 'C' or 'C\+\+'[^\n]*} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json: In JSON value '/10/output/dynamic/provideweak'} }
// { dg-regexp {[^\n]*p3595-config-wrong-type\.json:[0-9]+:[0-9]+: error: 'provideweak' must be a boolean[^\n]*} }

int f (int x) { return x; }
