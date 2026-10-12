// P3098: Postcondition captures on inline member functions (class body).
// Exercises access specifiers and captures of private members.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

struct Widget {
private:
  int counter = 0;

public:
  int next()
    post [old_c = counter] (r: r == old_c + 1)
  {
    return ++counter;
  }

  int peek() const
    post (r: r == counter)
  {
    return counter;
  }
};

class Account {
  double balance_ = 100.0;
  int txn_count_ = 0;

public:
  double withdraw(double amount)
    post [old_bal = balance_, old_txn = txn_count_, amount]
         (r: r == old_bal - amount && balance_ == r && txn_count_ == old_txn + 1)
  {
    balance_ -= amount;
    ++txn_count_;
    return balance_;
  }

  double balance() const { return balance_; }
  int transactions() const { return txn_count_; }
};

struct Multi {
private:
  int x_ = 1;
  int y_ = 2;

protected:
  int z_ = 3;

public:
  int combine()
    post [ox = x_, oy = y_, oz = z_] (r: r == ox + oy + oz)
  {
    int result = x_ + y_ + z_;
    x_ = y_ = z_ = 0;
    return result;
  }
};

int main() {
  Widget w;
  assert(w.next() == 1);
  assert(w.next() == 2);
  assert(w.peek() == 2);

  Account acct;
  assert(acct.withdraw(30.0) == 70.0);
  assert(acct.balance() == 70.0);
  assert(acct.transactions() == 1);

  Multi m;
  assert(m.combine() == 6);
}
