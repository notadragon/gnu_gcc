// P3098: Postcondition captures — destruction ordering.
// Captures are destroyed in reverse lexical order after predicates.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

int log_idx = 0;
int order_log[20];

struct Logger {
  int id;
  Logger(int i) : id(i) { order_log[log_idx++] = i; }
  Logger(const Logger& o) : id(o.id) { order_log[log_idx++] = id + 100; }
  ~Logger() { order_log[log_idx++] = id + 200; }
};

// Test destruction ordering with trivial predicates.
int f(int i)
  post [a = Logger(1), b = Logger(2)] (true)
  post [c = Logger(3)] (true)
{
  return i;
}

int main() {
  log_idx = 0;
  f(1);
  // Expected order:
  // 1 (construct a), 2 (construct b), 3 (construct c)   -- init phase
  // [predicates evaluated but no visible side effect]
  // 203 (destroy c), 202 (destroy b), 201 (destroy a)   -- destruction (reverse)
  assert(order_log[0] == 1);    // construct a
  assert(order_log[1] == 2);    // construct b
  assert(order_log[2] == 3);    // construct c
  assert(order_log[3] == 203);  // destroy c (last constructed, first destroyed)
  assert(order_log[4] == 202);  // destroy b
  assert(order_log[5] == 201);  // destroy a (first constructed, last destroyed)
  assert(log_idx == 6);
}
