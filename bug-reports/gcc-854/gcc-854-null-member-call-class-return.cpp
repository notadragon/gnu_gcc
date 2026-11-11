// GCC-854: -fsanitize=null does not check the object pointer of a member
// function call that returns a class with a non-trivial copy constructor
// or destructor by value; the same call returning int is checked.
// g++ -fsanitize=null gcc-854-null-member-call-class-return.cpp && ./a.out
extern "C" int dprintf (int, const char *, ...);
struct R { R () {} R (const R &) {} ~R () {} int r; };
struct S { R f () { return R (); } int g () { return 0; } };
__attribute__((noipa)) void cls (S *sp) { sp->f (); }
__attribute__((noipa)) void scalar (S *sp) { sp->g (); }
int main ()
{
  S *volatile sp = 0;
  cls (sp);
  dprintf (2, "cls done\n");
  scalar (sp);
}
