// A caller-side precondition check (-fcontracts-client-check=all) reading a
// by-value parameter whose type has a user-provided copy constructor and
// destructor compiles.
// { dg-do compile { target c++20 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontracts-client-check=all" }


struct S{
  S(){};
  S(const S&){}
  ~S(){};
  int x = 0;
};

void f(S s) pre(s.x == 1 ) {};

int main()
{
  S s;
  f(s);
}
