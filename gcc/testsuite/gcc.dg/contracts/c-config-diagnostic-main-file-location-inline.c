/* P3595 (GCC-967), C: c-config-diagnostic-main-file-location.c for the
   inline form, -fcontract-configuration=, whose diagnostics are located at
   <command-line>: here the unknown-key warning.  (A configuration error ends
   the compilation before the main file is read.)  The main file is not
   "included from" <command-line> (no include context on the next diagnostic,
   __INCLUDE_LEVEL__ 0), and the source's diagnostics are at their own lines
   in the main file.  */
/* { dg-do compile } */
/* { dg-options {-fcontracts-p4299 -fcontract-configuration=\[{"output":{"semantic":"enforce","x":1}}\]} } */
/* { dg-regexp {[^\n]*<command-line>: In JSON value '/0/output/x'\n<command-line>:1:[0-9]+: warning: unknown key 'x' in 'output' object[^\n]*\n[^\n]*c-config-diagnostic-main-file-location-inline\.c:14:2: warning: #warning "located"[^\n]*} } */

_Static_assert (__INCLUDE_LEVEL__ == 0, "main file");

#warning "located"

int x = ;			/* { dg-error "expected expression" } */
