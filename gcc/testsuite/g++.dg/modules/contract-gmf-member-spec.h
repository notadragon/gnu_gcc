// Header for contract-gmf-member-spec_[ab].C.
namespace N {
template <class T> struct A { template <class U> int f (int x); };
template <> template <class U> int A<int>::f (int x) pre (x > 0)
{ return x; }
}
