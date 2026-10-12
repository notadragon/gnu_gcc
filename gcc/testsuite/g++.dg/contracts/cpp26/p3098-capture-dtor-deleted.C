// P3098: a capture whose type has a deleted destructor is ill-formed.  The
// capture is destroyed when the call ends, so the type must be destructible
// from the context of the capture.
//
// This is the prvalue init-capture form, on a non-defining declaration --
// the shape that needs no copy at all, so it isolates destructibility.  The
// parameter-capture form, and the inaccessible-destructor case, are in
// p3098-capture-dtor-inaccessible.C; the trivial-destructor case that used to
// crash is in p3098-capture-trivial-dtor.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

struct NoDestroy {
  NoDestroy() = default;
  NoDestroy(const NoDestroy&) = default;
  ~NoDestroy() = delete;
};

void f(int i)
  post [nd = NoDestroy{}] (true); // { dg-error "deleted" }
