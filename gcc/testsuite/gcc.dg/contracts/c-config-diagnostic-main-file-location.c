/* P3595 (GCC-967), C: a diagnostic located in the contract configuration
   -- here the unknown-key warning -- does not change how the main file is
   entered: the configuration's diagnostics keep their locations in the
   configuration, the main file is not "included from" it (no include
   context on the next diagnostic, __INCLUDE_LEVEL__ 0), and the source's
   diagnostics are at their own lines in the main file.  The dg-regexp
   below takes the configuration's diagnostics and the #warning together,
   so a line between them (such as an include context) fails it.  The
   inline form: c-config-diagnostic-main-file-location-inline.c.  For
   C++: p3595-config-diagnostic-main-file-location*.C.  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-config-diagnostic-main-file-location.json" } */
/* { dg-regexp {[^\n]*c-config-diagnostic-main-file-location\.json: In JSON value '/0/match/module'\n[^\n]*c-config-diagnostic-main-file-location\.json:2:[0-9]+: warning: unknown key 'module' in 'match' object[^\n]*\n[^\n]*c-config-diagnostic-main-file-location\.c:17:2: warning: #warning "located"[^\n]*} } */

_Static_assert (__INCLUDE_LEVEL__ == 0, "main file");

#warning "located"

int x = ;			/* { dg-error "expected expression" } */
