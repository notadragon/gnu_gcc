// Watch test for a bug that is OPEN ON CLANG and correct here.
//
// [except.ctor]/2: "If an exception is thrown during the destruction of
// temporaries or local variables for a return statement, the destructor for
// the returned object (if any) is also invoked."  So when `guard` throws on
// the way out of `f`, the already-constructed return object must still be
// destroyed and `live` must be back to 0.
//
// GCC implements this deliberately -- cp/except.cc carries a
// current_retval_sentinel mechanism whose stated purpose is to "clean up the
// return value if a local destructor throws".  This test PASSES here, and
// pins that it keeps doing so.
//
// The MIRROR is clang/test/Contracts/OpenBugs/retval-throwing-cleanup.cpp,
// which is XFAILed: Clang has no equivalent mechanism and leaks the return
// object (CLANG-5 in that fork's bug-reports/).  Same test content, opposite
// expectation -- which is the point of mirroring it.
//
// NO CONTRACTS ARE INVOLVED; plain C++17, found as the no-contract control
// while fixing GCC-5.
//
// { dg-do run { target c++17 } }

int live = 0;

struct Counted
{
  int v;
  Counted (int x) : v (x) { ++live; }
  Counted (const Counted &o) : v (o.v) { ++live; }
  ~Counted () { --live; }
};

struct ThrowOnDestroy
{
  bool armed;
  ~ThrowOnDestroy () noexcept (false) { if (armed) throw 42; }
};

Counted
f (bool arm)
{
  ThrowOnDestroy guard { arm };
  Counted result (7);
  return result;                // built, then `guard` throws on the way out
}

int
main ()
{
  try { f (true); } catch (int) { }
  if (live != 0)                // GCC: 0.  Clang: 1.
    __builtin_abort ();
  return 0;
}
