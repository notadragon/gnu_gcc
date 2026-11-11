// GCC-962 reproducer, half a of a grouped test: copy both halves into
// gcc/testsuite/g++.dg/modules/ and run
//   make check-g++ RUNTESTFLAGS="modules.exp=gcc-962-multi-cmi-leak*"
// then list gcc/testsuite/g++/gcm.cache: std.gcm, std.compat.gcm, the
// <bits/stdc++.h> header unit's CMI and leak.gcm are all still there,
// although the group named each of them with dg-module-cmi.

// { dg-additional-options "-fmodules --compile-std-module" }
// { dg-do compile { target c++20 } }
// { dg-module-cmi std }
// { dg-module-cmi std.compat }
// { dg-module-cmi <bits/stdc++.h> }
// { dg-module-cmi leak }

export module leak;
import std;
export inline std::size_t n () { return 0; }
