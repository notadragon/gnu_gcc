// -ffile-prefix-map remaps the module name of a translation unit's globals
// and the one it passes to __asan_before_dynamic_init alike, so its own
// dynamic initialization is not reported as an initialization-order fiasco
// (pr77396-2.C, under the map).
// { dg-do run }
// { dg-options "-ffile-prefix-map==FILE-PREFIX" }
// { dg-set-target-env-var ASAN_OPTIONS "check_initialization_order=true" }

struct S { S () { asm volatile ("" : : : "memory"); } };
static S c;
static int a = 0;
static int b = a;

int
main ()
{
  return 0;
}
