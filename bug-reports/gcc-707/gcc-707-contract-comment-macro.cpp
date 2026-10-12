#include <contracts>

#define M 0
#define LT(a, b) ((a) < (b))
#define X x
#define ZERO 0
#define ID(e) e

// Each violation's comment should be its predicate as written.
int f (int x) pre (M > x) { return x; }
int g (int x) pre (LT (x, 0)) { return x; }
int h (int x) pre (x < ZERO) { return x; }
int i (int x) pre (X < ZERO) { return x; }
int j (int x) pre (ID (x) < 0) { return x; }

int main ()
{
  f (1); g (1); h (1); i (1); j (1);
}
