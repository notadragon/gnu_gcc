// P3098: Capture init exception with enforce semantic — calls handler then terminates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "contract violation" }

struct Bad {
  Bad(int) { throw 42; }
  ~Bad() {}
};

int f(int i)
  post [b = Bad(1)] (true)
{
  return i;
}

int main() {
  f(10);
  // Should not reach here — enforce terminates after handler.
  return 1;
}
// { dg-output "contract violation in function int f.int. at .*(\n|\r\n|\r)" }
// { dg-output ".assertion_kind: post_capture, semantic: enforce, mode: evaluation_exception.*(\n|\r\n|\r)" }
