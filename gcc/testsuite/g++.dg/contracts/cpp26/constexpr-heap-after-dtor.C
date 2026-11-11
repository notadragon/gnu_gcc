// GCC-710 (stock GCC too; not a contracts bug): using a heap object after
// an explicit destructor call -- reading a member, a virtual call, a
// conversion to a virtual base, or a delete through a virtual destructor --
// ICEd stock checking builds in cxx_eval_constant_expression (the
// gcc_checking_assert in the void_node fallback, also reached by PR107529's
// testcase).  Here the destroyed object is outside its lifetime (GCC-69/70),
// so each is rejected cleanly.  A dynamic_cast in the same position is
// GCC-752 (open-bug-constexpr-dynamic-cast-typeid-outside-lifetime.C).
// { dg-do compile { target c++26 } }

struct T { int x = 0; };
constexpr int
f ()
{
  T *p = new T;
  p->~T ();
  int r = p->x;		// { dg-error "outside its lifetime" }
  delete p;
  return r;
}
static_assert (f () == 0);	// { dg-error "non-constant condition" }

struct V { constexpr virtual int g () const { return 1; } constexpr virtual ~V () {} };
struct W : V {};
constexpr int
h1 ()
{
  V *p = new W;
  p->~V ();
  int r = p->g ();	// { dg-error "outside its lifetime" }
  delete p;
  return r;
}
static_assert (h1 () == 1);	// { dg-error "non-constant condition" }

struct VB { int v = 1; };
struct VD : virtual VB { constexpr ~VD () {} };
constexpr int
h3 ()
{
  VD *p = new VD;
  p->~VD ();
  VB *b = p;		// { dg-error "outside its lifetime" }
  (void) b;
  delete p;
  return 1;
}
static_assert (h3 () == 1);	// { dg-error "non-constant condition" }

constexpr int
h4 ()
{
  V *p = new W;
  p->~V ();
  delete p;		// { dg-error "outside its lifetime" }
  return 1;
}
static_assert (h4 () == 1);	// { dg-error "non-constant condition" }
