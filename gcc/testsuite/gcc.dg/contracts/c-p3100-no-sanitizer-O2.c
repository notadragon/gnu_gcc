/* GCC-965: -fcontracts-p3100 without a -fsanitize= option compiles C at
   -O2 (c-p3100-no-sanitizer.c).  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4299 -fcontracts-p3100 -O2" } */

void f (void) { }

int g (int a, int b) { return a / b + a * b; }

int h (int *p) { return *p; }
