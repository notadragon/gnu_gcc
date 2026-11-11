// -fsanitize=null does not check the object pointer of `->*'.
// g++ -fsanitize=null ptrmem-null-object-not-checked.cpp && ./a.out
// Expected: "runtime error: member access within null pointer of type 'S'"
// (or a load of a null pointer).  Seen: SIGSEGV, no report.
struct S { int a; int b; };
S *volatile np = nullptr;
int S::*volatile pm = &S::b;
__attribute__((noinline)) int ptm () { return np->*pm; }
int main () { return ptm (); }
