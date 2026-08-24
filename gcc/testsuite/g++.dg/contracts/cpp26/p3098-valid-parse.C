// P3098: Postcondition captures — valid syntax and capture visibility.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

int f1(int i)
  post [i] (r: r >= i);                  // simple param capture

int f2(int i, int j)
  post [i, j] (r: r >= i && r >= j);     // multiple param captures

int f3(int i)
  post [old_i = i] (r: r >= old_i);      // init-capture from param

int f4()
  post [x = 42] (r: r > x);             // init-capture from literal

int get_size();
void do_push(int item)
  post [old_size = get_size()] (get_size() == old_size + 1); // non-trivial expr

int f5(int i)
  post [i] (r: r >= i)
  pre (i >= 0);                          // capture before precondition

int f6(int i)
  post [a = i] (r: r > a)
  post [b = i + 1] (r: r > b);          // multiple posts with captures

int f7(int i)
  post [i] (r: r > i)
  post (r: r > 0);                       // mix of captured and captureless
