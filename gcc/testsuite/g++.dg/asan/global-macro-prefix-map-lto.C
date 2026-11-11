// Under LTO, the filename ASan records for a global is not remapped by
// -fmacro-prefix-map alone: the descriptor is built at link time from the
// streamed location, which only the debug prefix map has remapped (the
// limitation documented with -fsanitize=address).  Without LTO it is
// remapped (c-c++-common/asan/global-macro-prefix-map-1.c); under LTO
// -fdebug-prefix-map remaps it (global-debug-prefix-map-lto.C), as does
// -ffile-prefix-map (c-c++-common/asan/global-file-prefix-map-1.c).  Only
// the -O0 variant runs, with -flto added.
// { dg-do run }
// { dg-require-effective-target lto }
// { dg-skip-if "" { *-*-* } { "*" } { "-O0" } }
// { dg-options "-flto -fmacro-prefix-map==FILE-PREFIX" }
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
// { dg-output ".*XXX\[^\n\r]* defined in '(?!FILE-PREFIX)\[^\n\r]*global-macro-prefix-map-lto.C:16:6'" }
