// The object pointer of `->*' with a pointer to data member is checked
// before the member's offset is added to it (GCC-569).
// { dg-do run }
// { dg-options "-fsanitize=null -fno-sanitize-recover=null" }
// { dg-shouldfail "ubsan" }

struct S { int a; int b; };
S *volatile np = 0;
int S::*volatile pm = &S::b;

__attribute__((noinline)) int ptm () { return np->*pm; }

int main () { return ptm (); }

// { dg-output "member access within null pointer of type 'struct S'" }
