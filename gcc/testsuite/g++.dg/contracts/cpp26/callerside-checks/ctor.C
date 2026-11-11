// A constructor with a precondition, constructed with caller-side
// precondition checks (-fcontracts-client-check=pre), compiles.
// { dg-do compile { target c++20 } }
// { dg-additional-options "-fcontracts -fcontracts-client-check=pre" }

struct X
{
    X (int x) noexcept
     pre (x>1)
    {
      try {
       int i = 1;
      }
      catch(...) {
      }
    }
};

int main()
{
  try {
    X x(-42);
  } catch (...) {
  }
}
