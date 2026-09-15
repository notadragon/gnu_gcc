// A contract on a function returning a class with a non-trivial destructor
// causes the returned object to be destroyed twice when a local's destructor
// exits via an exception.
//
//   g++ -std=c++26 double-destroy.cpp -lstdc++exp && ./a.out; echo $?
//
//     -> 255   (live == -1: one construction, two destructions)
//     -> 0     expected, and what you get if `pre(true)' is deleted
//
// The predicate's content is irrelevant.  `post(true)' behaves the same way.

int live = 0;

struct Counted {
    int v;
    Counted(int x) : v(x) { ++live; }
    Counted(const Counted& o) : v(o.v) { ++live; }
    ~Counted() { --live; }
};

struct ThrowOnDestroy {
    bool armed;
    ~ThrowOnDestroy() noexcept(false) { if (armed) throw 42; }
};

Counted f(bool arm) pre(true)   // delete `pre(true)' and the bug goes away
{
    ThrowOnDestroy guard{arm};
    Counted result(7);
    return result;              // built, then `guard' throws on the way out
}

int main()
{
    try { f(true); } catch (int) { }
    return live;                // 0 expected; -1 (exit 255) as it stands
}
