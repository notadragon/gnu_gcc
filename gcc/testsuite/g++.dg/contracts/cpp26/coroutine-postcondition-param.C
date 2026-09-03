/* An odr-use of a non-reference parameter in a postcondition of a coroutine
   is ill-formed.

   [dcl.contract.func] requires such a parameter to be const.
   [dcl.fct.def.coroutine]/5 creates the coroutine's parameter copies "at the
   beginning of the replacement body", and says the copy of a non-reference
   parameter of type CV T is direct-initialized from an xvalue of type T --
   the UNQUALIFIED type, which cannot be formed from a const parameter.  The
   two requirements cannot both be met, and the standard states the
   consequence outright, as a note on [dcl.fct.def.coroutine]:

     "An odr-use of a non-reference parameter in a postcondition assertion of
      a coroutine is ill-formed."

   So writing the parameter non-const is rejected by the const rule, writing
   it const is rejected by this one, and there is no third spelling.  Use a
   reference parameter, whose frame copy [dcl.fct.def.coroutine]/5 binds to
   the same object.

   Neither compiler diagnosed the const spelling before; found by a
   contracts-x-coroutines sweep.  */

// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

#include <coroutine>

struct Task
{
  struct promise_type
  {
    Task get_return_object () { return {}; }
    std::suspend_always initial_suspend () noexcept { return {}; }
    std::suspend_always final_suspend () noexcept { return {}; }
    void return_void () { }
    void unhandled_exception () { }
  };
};

/* Non-const by value gets BOTH diagnostics, and the pair is the useful
   answer: the general rule says it must be const, and this one says const
   would not rescue it either because the function is a coroutine.  */
Task non_const (int x) post (x > 0) { co_return; }  // { dg-error "value parameter used in a postcondition must be const" }
// { dg-error "odr-used in a postcondition of a coroutine" "" { target *-*-* } .-1 }

/* Const by value: rejected because this is a coroutine.  */
Task by_value (const int x) post (x > 0) { co_return; }  // { dg-error "odr-used in a postcondition of a coroutine" }

/* A reference parameter is fine -- its copy is bound to the same object.  */
Task by_ref (const int &x) post (x > 0) { co_return; }
Task by_mutable_ref (int &x) post (x > 0) { co_return; }

/* A PREcondition may name a by-value parameter: the const rule, and so this
   restriction, are postcondition rules.  */
Task in_pre (int x) pre (x > 0) { co_return; }

/* A const by-value parameter with no postcondition naming it is unaffected;
   the restriction is on the odr-use, not on the parameter.  */
Task const_unused (const int x) post (true) { co_return; }

/* Naming the result binding rather than a parameter is fine.  */
Task result_only (int x) post (r : true) { co_return; }

/* A non-coroutine with the same signature keeps working -- this is what
   makes the restriction coroutine-specific rather than a general tightening
   of the const rule.  */
int not_a_coroutine (const int x) post (x > 0) { return x; }

/* The same restriction inside a template, where the function is only known
   to be a coroutine once its body is parsed.
   Deliberately NOT instantiated here: an instantiation repeats the same
   diagnostic at the same location (once for the template body, once for the
   instantiation), and one dg-error consumes both, so pinning the pair is
   more trouble than it is worth.  The duplication is a diagnostic-quality
   wrinkle, not a second property.  */
template <class T>
Task templated (const T x) post (x > 0) { co_return; }  // { dg-error "odr-used in a postcondition of a coroutine" }

