// P3100: a misaligned placement new that stores nothing
// (ub:basic.align.object.alignment.nostore) is reported when its pointer
// is arithmetic on a character array, `new (s + o) S ()', as through a
// pointer variable -- the empty class and default-initialization alike.
// Stock Clang's -fsanitize=alignment misses the arithmetic form (the
// mirror of llvm_llvm-project's test, M94-Clang); GCC's checks neither
// (GCC-877).  The forms are in p3100-placement-align-char-array.h.
// At -O2.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -O2" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:noexcept_observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include "p3100-placement-align-char-array.h"

// { dg-output "arith 1(\n|\r\n|\r)var 1(\n|\r\n|\r)arith_dflt 1(\n|\r\n|\r)" }
// { dg-output "var_dflt 1(\n|\r\n|\r)elem 1(\n|\r\n|\r)aligned 0(\n|\r\n|\r)" }
