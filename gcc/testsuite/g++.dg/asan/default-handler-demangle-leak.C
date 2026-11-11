// The default contract-violation handler frees the demangled name of an
// escaping exception's type: under observe it returns, so a leak would be
// reported by LeakSanitizer at exit.
//
// Mirror: clang/test/Contracts/default-handler-demangle-leak.cpp in the
// llvm_llvm-project fork.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

extern "C" const char *__asan_default_options () { return "detect_leaks=1"; }

struct some_exception {};
bool boom (int) { throw some_exception{}; }
int f (int x) pre (boom (x)) { return x; }

int main ()
{
  for (int i = 0; i < 3; ++i)
    f (i);
  return 0;
}
