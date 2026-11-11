// P3098: a capture whose type has an inaccessible destructor is ill-formed.
// The capture is destroyed at the end of the call, so the destructor must be
// accessible at the point of capture, exactly as for a local variable.
//
// This used to report once and then end the translation unit with "confused
// by earlier errors, bailing out" -- a masked ICE in build_cleanup, whose
// general form is covered by p3098-capture-trivial-dtor.C.
//
// Exactly one diagnostic, from the capture's copy-initialization.  The
// destructor is emitted once per exit path, so diagnosing from there as well
// would repeat the same error two or three times for one capture.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

bool ok (int);

struct PrivateDtor {
  int v = 1;
  PrivateDtor () = default;
  PrivateDtor (const PrivateDtor &) = default;
private:
  ~PrivateDtor () = default;
};

struct DeletedCopy {
  int v = 1;
  DeletedCopy () = default;
  DeletedCopy (const DeletedCopy &) = delete;
};

void f (PrivateDtor p) post [p] (ok (p.v)) { }	// { dg-error "private" }

// The translation unit keeps going, which is what the bail-out used to
// prevent: this later, unrelated error is still reported.
void later (DeletedCopy p) post [p] (ok (p.v)) { }  // { dg-error "deleted" }
