/* -fmacro-prefix-map remaps the filename recorded for a global, which an
   ASan report names, as it does __FILE__ (global-file-prefix-map-1.c for
   -ffile-prefix-map).  Not under LTO, where only the debug map applies:
   g++.dg/asan/global-macro-prefix-map-lto.C pins that.  */
/* { dg-do run } */
/* { dg-skip-if "LTO follows the debug map" { *-*-* } { "-flto" } { "" } } */
/* { dg-options "-fmacro-prefix-map==FILE-PREFIX" } */
/* { dg-shouldfail "asan" } */

volatile int ten = 10;
char XXX[10];

int
main ()
{
  XXX[ten - 10] = 1;
  return XXX[ten];
}

/* { dg-output "0x\[0-9a-f\]+ is located 0 bytes after global variable" } */
/* { dg-output ".*XXX\[^\n\r]* defined in 'FILE-PREFIX(?!FILE-PREFIX)\[^\n\r]*global-macro-prefix-map-1.c:11:6'" } */
