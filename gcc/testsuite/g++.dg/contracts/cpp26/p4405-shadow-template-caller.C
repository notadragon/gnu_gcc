// P4405 stage 3: a caller-side evaluation of a precondition of a
// specialization of a template with a definition is proven in the caller's
// proof shadow, with the call's arguments for the specialization's
// parameters, as for any other function: the predicate the marker holds is
// the substituted one (p4405-fold-caller-side-template.C), not the
// pattern's.  A function template, a member template, a member function of
// a class template and a pack; each proven where the caller tests the
// argument and diagnosed where it does not.  The definitions are not
// diagnosed: their own evaluations are ignore.
//
// Mirror: clang/test/Contracts/p4405-shadow-template-caller.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-shadow-caller-side.json" }

extern "C" int puts (const char *);

template <class P>
void log_message (P p)
  pre (p)			// { dg-message "the predicate of the contract assertion" }
{
  puts (p);
}

void output_string (const char *ptr)
{
  if (ptr != nullptr)
    log_message (ptr);		// { dg-bogus "cannot be proven|always false" }
}

void output_string_unchecked (const char *ptr)
{
  log_message (ptr);		// { dg-error "contract assertion cannot be proven" }
}

struct M {
  int n;
  template <class U>
  int at (U i) const pre (i < n) { return i; }	// { dg-message "the predicate of the contract assertion" }
};

int use_member (const M &m, long i)
{
  if (i < m.n)
    return m.at (i);		// { dg-bogus "cannot be proven|always false" }
  return m.at (i);		// { dg-error "contract assertion cannot be proven" }
}

template <class T>
struct C {
  T get (T i) pre (i > 0) { return i; }	// { dg-message "the predicate of the contract assertion" }
};

int use_class (C<int> *c, int i)
{
  if (i > 0)
    return c->get (i);		// { dg-bogus "cannot be proven|always false" }
  return c->get (i);		// { dg-error "contract assertion cannot be proven" }
}

template <class... T>
int all (T... t) pre (((t > 0) && ...)) { return 0; }	// { dg-message "the predicate of the contract assertion" }

int use_pack (int a, int b)
{
  if (a > 0 && b > 0)
    return all (a, b);		// { dg-bogus "cannot be proven|always false" }
  return all (a, b);		// { dg-error "contract assertion cannot be proven" }
}
