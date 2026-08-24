// P3097+P3098+P3099: a virtual function whose postcondition has BOTH a capture
// and a user-defined message.  The violation should carry the message and see
// the captured value.  (Previously BUG-3: GCC ICEd -- "expected string_cst,
// have view_convert_expr in build_contract_data_block_ctor" -- because the
// late-parse path stored the message without normalizing it to a bare
// STRING_CST.  Now fixed.)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int state = 0;
static int viol = 0;
static const char* last_msg = nullptr;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  ++viol;
  last_msg = v.message();
}

struct Base {
  virtual int f()
    post [old = state] (r: r == old + 1, "bad increment")
  { return ++state; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int f() override { return state += 2; }   // returns old+2, so post fails
};

int main() {
  state = 0;
  Derived d;
  Base& b = d;
  (void) b.f();   // interface post captures old=0, wants r==1, gets r==2 -> violation
  if (viol != 1) __builtin_abort();
  if (!last_msg || std::strcmp(last_msg, "bad increment") != 0) __builtin_abort();
}
