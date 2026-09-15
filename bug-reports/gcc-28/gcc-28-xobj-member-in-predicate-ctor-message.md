# GCC-28: A member named in an explicit-object member function's contract is diagnosed with a constructor/destructor message

**Status:** Fixed here (commit `a611ae28328`)
**Resolved by:** `1200-xobj-member-in-predicate`
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `diagnostic`
**Upstream Link:** [PR127294](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127294)
-- **FILED 2026-09-09**, UNCONFIRMED. (Searched 2026-09-05 before filing,
on the distinctive message text itself -- comment search for
`constructor precondition destructor postcondition contract check`, including
resolved bugs -- with no hits. That phrase appears verbatim only in GCC's own
source, so a report of this would almost certainly contain it.)
**Affects:** measured 2026-09-09 -- reproduces on 16.1.0, 16.2.0 and both
trunk builds. 13/14/15 do not accept this syntax.

## Bug Report

| Bugzilla field | Value |
|---|---|
| Product | `gcc` |
| Component | `c++` |
| Version | `16.2.0` |
| Severity | `normal` |
| Host / Target / Build | `x86_64-linux-gnu` |
| Summary | `[c++26][contracts] a member named unqualified in an explicit-object member function's contract is diagnosed as a constructor/destructor contract check` |

Attachments:

| File | Description |
|---|---|
| [`xobj-member-in-predicate-ctor-message.cpp`](xobj-member-in-predicate-ctor-message.cpp) | The four shapes given the wrong message, with four controls including a real constructor |

````
Naming a non-static data member unqualified in a contract predicate of an
explicit object member function is correctly rejected, but with a message
about a rule that does not apply -- the function is neither a constructor
nor a destructor.

The expected diagnostic is the one the function body already gives for the
same expression: "invalid use of non-static data member 'S::x'".  An
unqualified member means (*this).x, and an explicit object member function
has no this ([expr.prim.this]/1).

```
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
```

```
$ ./gcc-16.2.0/bin/g++ -std=c++26 -fsyntax-only \
    xobj-member-in-predicate-ctor-message.cpp
xobj-member-in-predicate-ctor-message.cpp: In explicit object member function 'void ImplicitThisPre::f(this ImplicitThisPre&)':
xobj-member-in-predicate-ctor-message.cpp:22:44: error: 'S::x' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   22 |     void f(this ImplicitThisPre& self) pre(x == 0);
      |                                            ^
xobj-member-in-predicate-ctor-message.cpp: In explicit object member function 'int ImplicitThisPost::f(this ImplicitThisPost&)':
xobj-member-in-predicate-ctor-message.cpp:27:49: error: 'S::x' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   27 |     int f(this ImplicitThisPost& self) post(r : x == r);
      |                                                 ^
xobj-member-in-predicate-ctor-message.cpp: In explicit object member function 'void ImplicitThisAssert::f(this ImplicitThisAssert&)':
xobj-member-in-predicate-ctor-message.cpp:34:25: error: 'S::x' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   34 |         contract_assert(x == 0);
      |                         ^
xobj-member-in-predicate-ctor-message.cpp: In explicit object member function 'void ImplicitThisCall::f(this ImplicitThisCall&)':
xobj-member-in-predicate-ctor-message.cpp:41:45: error: 'bool S::ok() const' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   41 |     void f(this ImplicitThisCall& self) pre(ok());
      |                                             ^~
xobj-member-in-predicate-ctor-message.cpp: In explicit object member function 'void ExplicitThis::f(this ExplicitThis&)':
xobj-member-in-predicate-ctor-message.cpp:47:41: error: 'this' is unavailable for explicit object member functions
   47 |     void f(this ExplicitThis& self) pre(this->x == 0);
      |                                         ^~~~
xobj-member-in-predicate-ctor-message.cpp:47:31: note: use explicit object parameter 'self' instead
   47 |     void f(this ExplicitThis& self) pre(this->x == 0);
      |            ~~~~~~~~~~~~~~~~~~~^~~~
xobj-member-in-predicate-ctor-message.cpp: In constructor 'RealCtor::RealCtor()':
xobj-member-in-predicate-ctor-message.cpp:65:20: error: 'S::x' 'this' required when accessing a member within a constructor precondition or destructor postcondition contract check
   65 |     RealCtor() pre(x == 0);
      |                    ^
```

Four shapes are affected -- a precondition, a postcondition, an
assertion-statement, and an unqualified member function call -- and the
message is the same wrong one in each.

ExplicitThis, spelling `this->x` outright, gets the right words in the
diagnostic: "'this' is unavailable for explicit object member functions",
with a note suggesting the explicit object parameter.  RealCtor, a
genuine constructor precondition, is where that constructor/destructor
message belongs and must keep producing it.

DISCOVERY

Found by an audit comparing every test in a C++26 contracts implementation's
own suite against stock trunk, looking for cases that pass locally only
because they were fixed locally.  The member-function-call shape (4) had no
test coverage at all on either side.

ANALYSIS

Two guards in the contracts code reach for the constructor/destructor
message when an unqualified member cannot be resolved in a predicate -- one
for a data member, one for a member function call.  Both assume the only way
to arrive there is from a constructor's precondition or a destructor's
postcondition.  An explicit object member function is a third way in, and
neither guard checks.

VERSIONS -- all on x86_64-linux-gnu

  source              version                       wrong message
  compiler-explorer   16.1.0                        yes
  compiler-explorer   16.2.0                        yes
  compiler-explorer   17.0.0 20260909, 919c0d16c91  yes
  local build -g      17.0.0 20260909, 7dab38c9d71  yes

```
$ ./gcc-16.2.0/bin/g++ -v
Using built-in specs.
COLLECT_GCC=./gcc-16.2.0/bin/g++
COLLECT_LTO_WRAPPER=.../gcc-16.2.0/bin/../libexec/gcc/x86_64-linux-gnu/16.2.0/lto-wrapper
Target: x86_64-linux-gnu
Configured with: ../gcc-16.2.0/configure --prefix=/opt/compiler-explorer/gcc-build/staging --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --disable-bootstrap --enable-multiarch --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --enable-clocale=gnu --enable-languages=c,c++,fortran,ada,objc,obj-c++,go,d,m2,rust,cobol,algol68 --enable-ld=yes --enable-gold=yes --enable-libstdcxx-time=yes --enable-linker-build-id --enable-lto --enable-plugins --enable-threads=posix --with-pkgversion=Compiler-Explorer-Build-gcc--binutils-2.44
Thread model: posix
Supported LTO compression algorithms: zlib
gcc version 16.2.0 (Compiler-Explorer-Build-gcc--binutils-2.44)
```
````

## Reproducer

See [`xobj-member-in-predicate-ctor-message.cpp`](xobj-member-in-predicate-ctor-message.cpp)
in this directory. It carries four affected shapes and four controls; the
controls matter, because two of them are what bound the defect:

* the explicit `this` spelling already produced the right message, and
* a **genuine constructor precondition** naming a member unqualified must keep
  producing the constructor message -- both compilers do, before and after.

## Our Fix

Test `contract_class_ptr` for non-nullness before comparing. The case then
falls through to the ordinary path and gets the same diagnostic the body
gives. Both guards in `finish_id_expression` have the defect and both are
fixed; the member-function one had no test coverage at all before.

Test: `gcc/testsuite/g++.dg/contracts/cpp26/deducing-this-no-this-in-predicate.C`.

## Notes

Found by a deliberate deducing-this x contracts sweep, which is also what
turned up [GCC-17](../gcc-17/gcc-17-this-in-xobj-declaration.md) -- `this` accepted in
the trailing return type of an explicit object member function. The two are
the contracts-specific and non-contracts halves of the same sentence in
[expr.prim.this]/3, and GCC-17's "found while fixing the contracts-specific
analog" means this entry.

A diagnostic-quality bug rather than an accepts-invalid one: the program is
correctly rejected either way. That is the same class as
[GCC-14](../gcc-14/gcc-14-contract-capture-note-garbage-location.md), which is tracked
on the same terms.
