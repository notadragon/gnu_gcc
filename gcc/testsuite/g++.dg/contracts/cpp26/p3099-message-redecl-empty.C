// P3099: an empty diagnostic message is a message, distinct from none
// (P3099R3 sec. "Empty string vs. no string": message() is "" for the one
// and null for the other).  So a redeclaration that adds or drops an empty
// message is mismatched, in either order, while two empty messages match --
// including an empty string literal and a user-defined message object whose
// extracted text is empty, since sameness is decided on the extracted text.
//
// Mirror: clang/test/Contracts/p3099-message-redecl-empty.cpp in the
// llvm_llvm-project fork; owed to EDG, recorded in its open-issues/README.md.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099" }

struct Empty
{
  constexpr int size () const { return 0; }
  constexpr const char *data () const { return ""; }
};

void f (int x) pre (x > 0, "");
void f (int x) pre (x > 0);		// { dg-error "mismatched contract diagnostic message" }

void g (int x) pre (x > 0);
void g (int x) pre (x > 0, "");		// { dg-error "mismatched contract diagnostic message" }

void h (int x) pre (x > 0, Empty{});
void h (int x) pre (x > 0);		// { dg-error "mismatched contract diagnostic message" }

void i (int x) pre (x > 0, "");
void i (int x) pre (x > 0, "");		// OK

void j (int x) pre (x > 0, "");
void j (int x) pre (x > 0, Empty{});	// OK, the same (empty) text
