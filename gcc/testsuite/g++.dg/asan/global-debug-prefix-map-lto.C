// Under LTO, the filename ASan records for a global follows the debug
// prefix map: the descriptor is built at link time from the streamed
// location, which -fdebug-prefix-map has remapped (the limitation
// documented with -fsanitize=address; -fmacro-prefix-map alone does not
// remap it, global-macro-prefix-map-lto.C).  Only the -O0 variant runs,
// with -flto added.
// { dg-do run }
// { dg-require-effective-target lto }
// { dg-skip-if "" { *-*-* } { "*" } { "-O0" } }
// { dg-options "-flto -fdebug-prefix-map==DEBUG-PREFIX" }
// { dg-shouldfail "asan" }

volatile int ten = 10;
char XXX[10];

int
main ()
{
  XXX[ten - 10] = 1;
  return XXX[ten];
}

// { dg-output "0x\[0-9a-f\]+ is located 0 bytes after global variable" }
// { dg-output ".*XXX\[^\n\r]* defined in 'DEBUG-PREFIX(?!DEBUG-PREFIX)\[^\n\r]*global-debug-prefix-map-lto.C:14:6'" }
