// `this' is accepted in the trailing return type of an explicit object
// member function, where [expr.prim.this]/3 makes it ill-formed.
//
//   g++ -std=c++23 -fsyntax-only this-in-xobj-trailing-return.cpp
//
//     -> the only diagnostic is for Control::g, the static member function.
//        Bug::f is accepted, and should not be.
//
// [expr.prim.this]/3, as amended by P0847R7 (deducing this):
//
//   "It shall not appear within the declaration of either a static member
//    function or an explicit object member function of the current class
//    (although its type and value category are defined within such member
//    functions as they are within an implicit object member function)."
//
// One sentence, two kinds of function.  A trailing return type is within the
// declaration, so `this' is ill-formed in it for both -- and the static half
// is rejected while the explicit-object half is not.
//
// No contracts involved; plain C++23.

struct Bug {
    int x;
    // Accepted, and should not be.
    auto f(this Bug& self) -> decltype(this->x);
};

struct Control {
    int x;
    // The same shape on a static member function, governed by the same
    // sentence.  Rejected -- so the rule is implemented, and applied to
    // only half of what it says.
    static auto g() -> decltype(this->x);
};

struct MustCompile {
    int x;
    // An implicit object member function may of course name `this' here.
    auto h() -> decltype(this->x);
};
