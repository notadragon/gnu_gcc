// gcc-952-assume-member-ipa-sra-ice.cpp                             -*-C++-*-
//
// GCC-952: an assumption in a member function that reads a data member ICEs
// in modify_call when ipa-sra rewrites the .ASSUME call.
//
//   g++ -std=c++23 -O2 -c gcc-952-assume-member-ipa-sra-ice.cpp
//
// Stock g++ trunk (since the 2026-10-27 series giving .ASSUME calls
// callback edges, 684c9811dee and its predecessors): internal compiler
// error: in modify_call, at ipa-param-manipulation.cc:762, during IPA pass
// inline.  -fno-ipa-sra avoids it.  16.2.0 compiles it.
//
// No contracts involved.

[[gnu::noinline]] bool check (int i) { return i > 0; }

struct S
{
  int v = 1;
  // THE BUG.
  [[gnu::noinline]] int m (int i) { [[assume (check (i + v))]]; return i + v; }
};

int main ()
{
  S s;
  return s.m (2) != 3;
}
