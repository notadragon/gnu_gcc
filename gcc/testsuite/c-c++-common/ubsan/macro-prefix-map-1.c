/* -fmacro-prefix-map remaps the filename in a UBSan report, as it does
   __FILE__ (file-prefix-map-1.c for -ffile-prefix-map).  */
/* { dg-do run } */
/* { dg-options "-fsanitize=integer-divide-by-zero -fmacro-prefix-map==FILE-PREFIX" } */

int
main (void)
{
  volatile int a = 1, b = 0;
  a / b;
  return 0;
}

/* { dg-output "^FILE-PREFIX(?!FILE-PREFIX)\[^\n\r]*macro-prefix-map-1.c:10:\[0-9\]+: runtime error: division by zero" } */
