struct Y {
  void f () pre ([] { return true; } ());
  template <int N> void g () post (r : r);
};
