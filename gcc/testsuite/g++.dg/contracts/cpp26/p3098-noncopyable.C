// P3098: Captures of non-destructible types — ill-formed.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

struct NoDestroy {
  NoDestroy() = default;
  NoDestroy(const NoDestroy&) = default;
  ~NoDestroy() = delete;
};

void f(int i)
  post [nd = NoDestroy{}] (true); // { dg-error "deleted" }
