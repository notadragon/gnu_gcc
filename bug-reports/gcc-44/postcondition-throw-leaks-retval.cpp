// A contract-violation handler that throws out of a postcondition leaks the
// returned object.  By the time a postcondition is evaluated the returned
// object has been initialized, and after unwinding the program can no longer
// reach it, so its destructor never runs.
//
//   g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce
//       postcondition-throw-leaks-retval.cpp -lstdc++exp && ./a.out; echo $?
//
//     -> 2  live == 2: three constructions (x,f,g), only one destruction (x)
//     -> 0  expected, and what you get with the post() deleted

#include <contracts>

int live = 0;

struct Counted {
    Counted () { ++live; }
    Counted (const Counted &) { ++live; }
    ~Counted () { --live; }
};

struct E { };

void handle_contract_violation (const std::contracts::contract_violation &)
{
    throw E { };
}

// normal exception throw with NRVO
Counted x ()
{
    Counted result;
    throw E{};
    return result;
}

// The postcondition fails, so the handler runs and leaves by exception.
Counted f (const int n) post (r : n > 100)
{
    // NRVO
    Counted result;
    return result;
}

// The postcondition fails, so the handler runs and leaves by exception.
Counted g (const int n) post (r : n > 100)
{
    // normal return (RVO)
    return Counted {};
}

int main ()
{
    try { x (); } catch (E &) { }
    if (live != 0) __builtin_abort();  // normal exceptions always clean up

    try { f (1); } catch (E &) { }
    try { g (1); } catch (E &) { }

    // 0 expected -- the returned object should have been destroyed while
    // unwinding.  >0 implies leaks.
    return live;
}
