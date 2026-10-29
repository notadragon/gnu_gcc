// A placement new at a misaligned address that stores nothing -- an empty
// class, default-initialization -- whose pointer is arithmetic on a
// character array, `new (s + o) S ()', and the same through a pointer
// variable, for the p3100-placement-align-char-array*.C tests: each run
// prints the number of implicit-assertion reports of one form.  The
// mirror of llvm_llvm-project's test of stock Clang's
// -fsanitize=alignment miss of the arithmetic form (M94-Clang); GCC's
// -fsanitize=alignment checks neither form (GCC-877).

#include <contracts>
#include <cstdio>
#include <new>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
struct alignas (4) S {};
struct T { int a, b; };
NI void arith (int o)
{
  alignas (S) char s[sizeof (S) + 1];
  new (s + o) S ();
}
NI void var (int o)
{
  alignas (S) char s[sizeof (S) + 1];
  void *m = s + o;
  new (m) S ();
}
NI void arith_dflt (int o)
{
  alignas (T) char s[sizeof (T) + 1];
  new (s + o) T;
}
NI void var_dflt (int o)
{
  alignas (T) char s[sizeof (T) + 1];
  void *m = s + o;
  new (m) T;
}
NI void elem (int o)
{
  alignas (S) char s[sizeof (S) + 1];
  new (&s[o]) S;
}

#define COUNT(name, ...)					\
  do								\
    {								\
      int before = reports;					\
      __VA_ARGS__;						\
      std::printf (name " %d\n", reports - before);		\
    }								\
  while (0)

int main ()
{
  int volatile one = 1, zero = 0;
  COUNT ("arith", arith (one));
  COUNT ("var", var (one));
  COUNT ("arith_dflt", arith_dflt (one));
  COUNT ("var_dflt", var_dflt (one));
  COUNT ("elem", elem (one));
  COUNT ("aligned", arith (zero); var (zero); arith_dflt (zero);
	 var_dflt (zero); elem (zero));
  return 0;
}
