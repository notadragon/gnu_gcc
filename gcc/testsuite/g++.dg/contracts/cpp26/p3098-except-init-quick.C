// P3098: Capture init exception with quick_enforce semantic — terminates directly.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "terminate" }

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
  return 1;
}
