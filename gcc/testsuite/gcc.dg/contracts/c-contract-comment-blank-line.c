/* The comment of a contract whose predicate spans several lines is its
   source text with each line break, and the blanks that begin the next
   line, read as one space.  A line holding nothing but blanks used to ICE
   in get_source_text_between, which the C front end shares with C++
   (PR127857, GCC-706).  */
/* { dg-do run } */
/* { dg-options "-fcontracts-p4299 -fcontract-evaluation-semantic=observe" } */

#include <contracts.h>
#include <string.h>

static const char *expected;
static int handler_called = 0;

void handle_contract_violation(const contract_violation_t* cv)
{
  const char* comment = stdc_contract_violation_comment(cv);
  if (!comment || strcmp(comment, expected) != 0)
    __builtin_abort();
  handler_called++;
}

int pre_spaces(int x) _Pre(x
  
  > 0)
{
  return x;
}

int post_tab(int x) _Post(r: r
	
	> 0)
{
  return x;
}

int main(void)
{
  expected = "x > 0";
  pre_spaces(-1);

  expected = "r > 0";
  post_tab(-1);

  int x = -1;
  expected = "x < 0 && x > 0";
  _ContractAssert(x < 0
  
		  && x > 0);

  if (handler_called != 3)
    __builtin_abort();

  return 0;
}
