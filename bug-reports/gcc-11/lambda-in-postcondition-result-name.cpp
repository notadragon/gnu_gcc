// A lambda-expression in the predicate of a free function's postcondition
// fails to parse when the postcondition uses a result-name-introducer.
//
//   g++ -std=c++26 -c lambda-in-postcondition-result-name.cpp
//
//     -> error: expected ')' before '{' token
//        error: expected unqualified-id before ')' token
//
// No capture is involved: the lambda below captures nothing.  This is a
// parse failure, not a capture problem.
//
// Rows (2) and (3) are accepted everywhere, so neither postconditions nor
// lambdas in predicates are broken as such -- it takes the combination of a
// free function, a postcondition, and a result-name-introducer.
//
// Row (4) is version-dependent; it is commented out so that this file's
// behaviour does not change with the compiler.  See its own comment.

bool ok() { return true; }

// (1) THE BUG: free function, postcondition, result-name-introducer.
int broken(int x) post(r : [] { return ok(); }()) { return x; }

// (2) Control: free function, postcondition, NO result name.  Accepted.
int no_result_name(int x) post([] { return ok(); }()) { return x; }

// (3) Control: free function, precondition.  Accepted.
int precondition(int x) pre([] { return ok(); }()) { return x; }

// (4) Control: member function, postcondition, result-name-introducer.
//     Version-dependent, which is why it is commented out: 16.1.0 and
//     16.2.0 reject this too, with a DIFFERENT diagnostic ("expected
//     conditional-expression"), while trunk accepts it.  The member-function
//     half was fixed by PR c++/125537 (r17-3710, commit 943089ca86f), which
//     narrowed the same raise at the other of its two sites; the
//     free-function half, row (1), was not.  Uncomment to see whichever
//     your compiler does.
//
// struct S {
//     int mem(int x) post(r : [] { return ok(); }()) { return x; }
// };

int main() { }
