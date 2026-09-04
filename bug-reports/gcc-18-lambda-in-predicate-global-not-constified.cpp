// [expr.prim.id.unqual]/3+e, the lambda half of the paper's example.
int n = 0;
struct X { bool m(); };
struct Y {
  int z = 0;
  void f(int i, int* p, int& r, X x, X* px)
    pre([=,&i,*this] mutable {
      ++n;         // error: attempting to modify const lvalue
      ++i;         // error: attempting to modify const lvalue
      ++p;         // OK, refers to member of closure type
      ++r;         // OK, refers to non-reference member of closure type
      ++this->z;   // OK, captured *this
      ++z;         // OK, captured *this
      return true;
    }())
  {}
};
