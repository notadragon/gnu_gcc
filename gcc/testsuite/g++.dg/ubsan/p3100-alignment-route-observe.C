// P3100 Task 4.1 (UBSan runtime routing): the alignment check
// (-fsanitize=alignment, ErrorType::MisalignedPointerUse) routes to the
// contract-violation handler.  alignment is recoverable, so with
// -fcontracts-p4298 the default -fsanitize=alignment resolves to
// noexcept_observe: the handler runs and the program CONTINUES.  Assert
// kind() == implicit (7) and semantic() == noexcept_observe (6).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=alignment" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

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
  int *p = reinterpret_cast<int *> (buf + 1);  // one byte off -> misaligned
  volatile int sink = load (p);
  (void) sink;
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// RF5: the sanitizer emits NOTHING on the routed path, so the handler line is
// the FIRST output -- anchor at ^ so any leaked pre-handler UBSan text fails.
// { dg-output "^handler kind=7 semantic=6" }
// noexcept_observe continues: "survived" prints after the detected access.
// { dg-output ".*survived" }
