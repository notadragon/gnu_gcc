// P3098: Postcondition captures — lexical interleaving with preconditions.
// Capture init happens in lexical order relative to precondition checks.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

int order_log[10];
int log_idx = 0;

struct Logger {
  int id;
  Logger(int i) : id(i) { order_log[log_idx++] = i; }
  Logger(const Logger& o) : id(o.id) { order_log[log_idx++] = id; }
  ~Logger() { order_log[log_idx++] = id + 200; }
};

bool check_pre(int tag) {
  order_log[log_idx++] = tag + 100;
  return true;
}

// Lexical order: post [capture](1) pre(2) post [capture](3)
// Expected: capture_1 init, pre_2 check, capture_3 init
int f(int i)
  post [a = Logger(1)] (true)
  pre (check_pre(2))
  post [b = Logger(3)] (true)
{
  return i;
}

int main() {
  log_idx = 0;
  f(1);
  // Expected order:
  //   1 (capture a init)
  //   102 (pre check — check_pre(2) returns true, logs 102)
  //   3 (capture b init)
  //   [function body]
  //   [predicates — no observable effect since trivial]
  //   203 (destroy b)
  //   201 (destroy a)
  assert(order_log[0] == 1);    // capture a constructed
  assert(order_log[1] == 102);  // precondition check
  assert(order_log[2] == 3);    // capture b constructed
  // Remaining entries are destructions (reverse order)
  assert(order_log[3] == 203);  // destroy b
  assert(order_log[4] == 201);  // destroy a
}
