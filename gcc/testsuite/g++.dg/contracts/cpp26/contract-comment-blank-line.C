// The comment of a contract whose predicate spans several lines is its source
// text with each line break, and the blanks that begin the next line, read
// as one space.  A line holding nothing but blanks used to ICE in
// get_source_text_between (PR127857, GCC-706); an empty line is skipped.
// Covers the four places a predicate is parsed: a precondition, a
// postcondition, contract_assert, and a member function's contracts, which
// are parsed at the end of the class.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

const char *expected;
int violations;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (std::strcmp (v.comment (), expected) != 0)
    std::abort ();
  ++violations;
}

// A line of two spaces.
int pre_spaces (int x) pre (x
  
  > 0) { return x; }

// A line of a tab.
int pre_tab (int x) pre (x
	
	> 0) { return x; }

// Spaces and a tab, and two blank lines in a row.
int pre_mixed (int x) pre (x
 	 
  
  > 0) { return x; }

// An empty line, which was always skipped.
int pre_empty (int x) pre (x

  > 0) { return x; }

// A continuation line that is not indented still gets its space.
int pre_flush (int x) pre (x
> 0) { return x; }

int post_spaces (int x) post (r: r
  
  > 0) { return x; }

struct S
{
  int member (int x) pre (x
  
    > 0) { return x; }
};

int main ()
{
  expected = "x > 0";
  pre_spaces (-1);
  pre_tab (-1);
  pre_mixed (-1);
  pre_empty (-1);
  pre_flush (-1);
  S ().member (-1);

  expected = "r > 0";
  post_spaces (-1);

  int x = -1;
  expected = "x < 0 && x > 0";
  contract_assert (x < 0
  
		   && x > 0);

  if (violations != 8)
    std::abort ();
}
