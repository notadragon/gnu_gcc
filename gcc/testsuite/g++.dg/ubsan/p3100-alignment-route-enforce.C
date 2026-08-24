// P3100 Task 4.1 (UBSan runtime routing): with -fcontracts-p4298 and
// -fno-sanitize-recover=alignment the routed alignment check resolves to
// noexcept_enforce: the handler runs (kind 7, semantic 7) and the program then
// TERMINATES.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=alignment -fno-sanitize-recover=alignment" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "noexcept_enforce terminates after the handler" }

#include <contracts>
#include <cstdio>

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

int __attribute__((noinline))
load (int *p)
{
  return *p;  // alignment: *p read through a misaligned pointer.
}

int main ()
{
  alignas (int) char buf[8];
  int *p = reinterpret_cast<int *> (buf + 1);
  volatile int sink = load (p);
  (void) sink;
  std::printf ("survived\n");  // must NOT be reached
  std::fflush (stdout);
  return 0;
}

// Handler ran with kind 7 (implicit) and semantic 7 (noexcept_enforce), then
// the program terminated (dg-shouldfail), so "survived" is never reached.
// { dg-output "^handler kind=7 semantic=7" }
