// GCC-734: -fsanitize=vptr does not check static_cast<D &> (b).
// Flags: -fsanitize=vptr
struct S { virtual ~S () {} int s = 1; };
struct T : S { int t = 2; };
struct U : S { int u = 3; };
__attribute__((noipa)) T *ptr (S *p) { return static_cast<T *> (p); }
__attribute__((noipa)) T &ref (S &r) { return static_cast<T &> (r); }
int main ()
{
  U u;
  ref (u);	// a downcast in reference form: not reported
  ptr (&u);	// the same downcast in pointer form: reported
  return 0;
}
