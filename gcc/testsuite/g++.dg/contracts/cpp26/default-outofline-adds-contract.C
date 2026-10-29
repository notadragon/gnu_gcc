// An out-of-line explicitly defaulted special member whose defaulting
// declaration adds contracts is ill-formed; the predicates are parsed and
// compared before the function is synthesized.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S { S (); };
S::S () pre (true) = default;		// { dg-error "adds contracts" }

struct D { ~D (); };
D::~D () pre (true) = default;		// { dg-error "adds contracts" }
