// Naming a non-static data member unqualified in a contract predicate of an
// explicit object member function is rejected with a message about
// constructors and destructors, for a function that is neither.
//
//   g++ -std=c++26 -fsyntax-only \
//       xobj-member-in-predicate-ctor-message.cpp
//
//     -> error: 'S::x' 'this' required when accessing a member within a
//        constructor precondition or destructor postcondition contract check
//
// Expected: the diagnostic the function body gives for the same expression --
// "invalid use of non-static data member 'S::x'", or for a member function
// call "cannot call member function ... without object".

struct S {
    int x = 0;
    bool ok() const;
};

// (1) Unqualified data member in a precondition.
struct ImplicitThisPre : S {
    void f(this ImplicitThisPre& self) pre(x == 0);
};

// (2) The same in a postcondition.
struct ImplicitThisPost : S {
    int f(this ImplicitThisPost& self) post(r : x == r);
};

// (3) The same in an assertion-statement in the body.
struct ImplicitThisAssert : S {
    void f(this ImplicitThisAssert& self)
    {
        contract_assert(x == 0);
    }
};

// (4) An unqualified member function call in a predicate -- the second of the
// two guards carrying the same defect.
struct ImplicitThisCall : S {
    void f(this ImplicitThisCall& self) pre(ok());
};

// CONTROL: the explicit `this' spelling was always right --
// "'this' is unavailable for explicit object member functions".
struct ExplicitThis : S {
    void f(this ExplicitThis& self) pre(this->x == 0);
};

// CONTROL: naming the member through the explicit object parameter is of
// course fine, and must keep compiling.
struct ViaSelf : S {
    void f(this ViaSelf& self) pre(self.x == 0) pre(self.ok());
};

// CONTROL: an ordinary implicit-object member function may name `this' and
// its members unqualified.
struct ImplicitObject : S {
    void g() const pre(this->x == 0) pre(x == 0);
};

// CONTROL: a genuine constructor precondition is where that message belongs,
// and it must keep being produced there.
struct RealCtor : S {
    RealCtor() pre(x == 0);
};
