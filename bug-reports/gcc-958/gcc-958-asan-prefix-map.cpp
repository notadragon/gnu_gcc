// GCC-958 (ASan): build with
//   -fsanitize=address -ffile-prefix-map=/src=/mapped
// and run.  __FILE__, printed first, is "/mapped/repro.cpp"; ASan's
// description of the global XXX says it is "defined in" "/src/repro.cpp",
// the same description a global-buffer-overflow report on XXX prints.  The
// #line directive gives the file a fixed name to map, independent of where
// this directory is checked out.
#include <stdio.h>
#include <sanitizer/asan_interface.h>
#line 1 "/src/repro.cpp"
char XXX[10];

int
main (void)
{
  fprintf (stderr, "__FILE__ is %s\n", __FILE__);
  __asan_describe_address (XXX + 10);
  return 0;
}
