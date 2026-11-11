// A reference bound to `*(int *) (a + o)', a an array of char and o an
// offset that misaligns it, for the p3100-align-binding-arith*.C tests:
// each run prints the number of implicit-assertion reports of one form.
// Nothing is read or written through the reference.

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation (const cs::contract_violation &v)
{
  if (v.kind () == cs::assertion_kind::implicit)
    ++reports;
}

#define NI __attribute__((noinline))
alignas (8) char buf[16];
// A global array plus an offset.
NI bool garr (int o) { int &r = *(int *) (buf + o); return &r == nullptr; }
// A local array plus an offset.
NI bool larr (int o)
{
  alignas (8) char lb[16];
  int &r = *(int *) (lb + o);
  return &r == nullptr;
}
// A call's argument.
NI void take (const int &) {}
NI void garg (int o) { take (*(int *) (buf + o)); }
// Controls: the address of an element, and a pointer plus an offset.
NI bool gsub (int o) { int &r = *(int *) &buf[o]; return &r == nullptr; }
NI bool gptr (char *c, int o) { int &r = *(int *) (c + o); return &r == nullptr; }

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
  int volatile one = 1, four = 4;
  COUNT ("garr", garr (one));
  COUNT ("larr", larr (one));
  COUNT ("garg", garg (one));
  COUNT ("gsub", gsub (one));
  COUNT ("gptr", gptr (buf, one));
  COUNT ("aligned", garr (four); larr (four); garg (four); gsub (four);
	 gptr (buf, four));
  return 0;
}
