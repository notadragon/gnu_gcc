// P3100 Task 2.1: under -fcontracts-p3100, resolving the address check to
// assume suppresses ASan instrumentation entirely -- byte-identical to a build
// without -fsanitize=address for that check.  No __asan_report* calls are
// emitted.

// { dg-do compile }
// -ffat-lto-objects forces real assembly for the -flto torture variants too,
// so scan-assembler-not is meaningful under every config (assume must suppress
// instrumentation under -flto exactly as non-LTO).
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fsanitize-semantic=address:assume -ffat-lto-objects" }

int sink;

int __attribute__((noinline))
oob (int *p, int i)
{
  return p[i];
}

int main ()
{
  int a[4] = { 0, 0, 0, 0 };
  sink = oob (a, 100);
  return 0;
}

// assume = check off: no ASan instrumentation is emitted.
// { dg-final { scan-assembler-not "__asan_report" } }
