// [class.mem.general]p7: a contract assertion on a member function is a
// complete-class context, so its predicate may name members declared later in
// the class -- exactly as a member function body may.
//
// GCC has always got this right; this is a guard mirrored from Clang, which
// stopped honouring it the moment anything stood between the declarator and
// the contract.  Clang cached a member's contract tokens on the strength of
// the token sitting directly after the declarator, and both a virt-specifier
// and a trailing requires-clause are parsed after that point, so a contract
// written behind either was parsed eagerly and every member declared after it
// became "use of undeclared identifier" (llvm, mirror ledger #62).
//
// The cases are organised by WHAT SEPARATES THE DECLARATOR FROM THE CONTRACT,
// not by virtualness: Virtual below is a virtual function that Clang accepted,
// and Override is the same function with `override` written on it, which it
// did not.  Every case names a member declared after the contract, since a
// case naming an earlier member would pass either way.

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3850" }

struct Base {
  virtual ~Base();
  virtual int vf() const;
  virtual int wf() const;
};

// --- Nothing in between: controls. ---------------------------------------

struct Plain {
  int f() const pre(g() >= 0);
  int g() const;
};

struct PlainDtor {
  ~PlainDtor() pre(g() >= 0);
  int g() const;
};

struct Virtual : Base {
  virtual int vf() const pre(g() >= 0);
  int g() const;
};

// --- A virt-specifier in between. ----------------------------------------

struct Override : Base {
  int vf() const override pre(g() >= 0);
  int g() const;
};

struct Final : Base {
  int vf() const final pre(g() >= 0);
  int g() const;
};

struct OverrideDtor : Base {
  ~OverrideDtor() override pre(g() >= 0);
  int g() const;
};

struct OverridePost : Base {
  int vf() const override post(r : r >= h());
  int h() const;
};

struct OverrideBoth : Base {
  int vf() const override pre(g() >= 0) post(r : r <= h());
  int g() const;
  int h() const;
};

struct OverrideNoexcept : Base {
  int vf() const noexcept override pre(g() >= 0);
  int g() const;
};

struct OverrideTrailingReturn : Base {
  auto vf() const -> int override pre(g() >= 0);
  int g() const;
};

struct OverrideDefinition : Base {
  int vf() const override pre(g() >= 0) { return g(); }
  int g() const;
};

struct OverridePure : Base {
  int vf() const override pre(g() >= 0) = 0;
  int g() const;
};

// The deferral is per-declarator, so a member-declarator-list must not lose it
// for the second and later declarators.
struct OverrideAfterOtherMembers : Base {
  int a, b;
  int vf() const override pre(g() >= 0);
  int g() const;
};

template <class X>
struct OverrideInClassTemplate : Base {
  int vf() const override pre(g() >= 0);
  int g() const;
};

template struct OverrideInClassTemplate<int>;

// Two overriders in one class, so the second is parsed with the first already
// deferred.
struct TwoOverriders : Base {
  int vf() const override pre(g() >= 0);
  int wf() const override post(r : r <= h());
  int g() const;
  int h() const;
};

// --- A trailing requires-clause in between. -------------------------------

struct Requires {
  template <class T>
  int f() const requires (sizeof(T) > 0) pre(g() >= 0);
  int g() const;
};

struct RequiresPost {
  template <class T>
  int f() const requires (sizeof(T) > 0) post(r : r <= h());
  int h() const;
};

// --- Positive control: the named member comes first, so no deferral is
// needed to find it.  This isolates the defect to WHEN the contract is parsed
// rather than to what is visible from it. -----------------------------------

struct MemberDeclaredFirst : Base {
  int k() const;
  int vf() const override pre(k() >= 0);
};
