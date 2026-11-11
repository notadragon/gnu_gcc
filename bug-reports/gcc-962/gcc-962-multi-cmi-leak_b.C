// GCC-962 reproducer, half b: see gcc-962-multi-cmi-leak_a.C.

// { dg-additional-options "-fmodules" }
// { dg-do compile { target c++20 } }

import leak;
int i = n ();
