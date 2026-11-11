/* GCC-965: -fcontracts-p3100 without a -fsanitize= option compiles C: the
   ubsan pass, which runs for P3100 alone, used to declare the sanitizer
   builtins after the C front end's file scope had gone and crash there.
   (c-p3100-no-sanitizer-O2.c: the same at -O2.)  */
/* { dg-do compile } */
/* { dg-options "-std=c23 -fcontracts-p4299 -fcontracts-p3100" } */

void f (void) { }

int g (int a, int b) { return a / b + a * b; }

int h (int *p) { return *p; }
