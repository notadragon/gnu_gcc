// P3595 (GCC-967): a diagnostic located in the contract configuration --
// here the unknown-key warning -- does not change how the main file is
// entered: the configuration's diagnostics keep their locations in the
// configuration, the main file is not "included from" it (no include
// context on the next diagnostic, __INCLUDE_LEVEL__ 0), and the source's
// diagnostics are at their own lines in the main file.  The dg-regexp
// below takes the configuration's diagnostics and the #warning together,
// so a line between them (such as an include context) fails it.
// The inline form: p3595-config-diagnostic-main-file-location-inline.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3595-config-diagnostic-main-file-location.json" }
// { dg-regexp {[^\n]*p3595-config-diagnostic-main-file-location\.json: In JSON value '/0/match/module'\n[^\n]*p3595-config-diagnostic-main-file-location\.json:2:[0-9]+: warning: unknown key 'module' in 'match' object[^\n]*\n[^\n]*p3595-config-diagnostic-main-file-location\.C:17:2: warning: #warning "located"[^\n]*} }

static_assert (__INCLUDE_LEVEL__ == 0);

#warning "located"

int x = ;			// { dg-error "expected primary-expression" }
