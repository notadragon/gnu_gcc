// The forms of the p3100-routed-handler-effects-builtin*.C tests: with
// -fsanitize=shift, signed-integer-overflow and integer-divide-by-zero
// routed to the contract-violation handler with noexcept_observe, each
// prints the number of reports of one check as `reports', a static only
// the handler changes, counts them.  A shift, a signed addition,
// subtraction, multiplication and negation, a division overflowing and a
// division by zero, in functions that are not inlined (the caller reads
// `reports' after the call) and in the caller itself.  The shift and
// division checks call a __ubsan_handle_* function; the signed overflow
// checks are IFN_UBSAN_CHECK_* internal calls until RTL expansion.  A
// division, which traps after the report, is caught as SIGFPE and resumed
// with siglongjmp.

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
}

static sigjmp_buf env;
static void on_fpe (int) { siglongjmp (env, 1); }

#define NI __attribute__((noinline))
NI int shl (int a, int b) { return a << b; }
NI int add (int a, int b) { return a + b; }
NI int sub (int a, int b) { return a - b; }
NI int mul (int a, int b) { return a * b; }
NI int neg (int a) { return -a; }
NI int dvo (int a, int b) { return a / b; }
NI int dvz (int a, int b) { return a % b; }

#define COUNT(name, ...)					\
  do								\
    {								\
      int before = reports;					\
      __VA_ARGS__;						\
      std::printf (name " %d\n", reports - before);		\
    }								\
  while (0)

#define COUNT_TRAP(name, ...)					\
  do								\
    {								\
      int before = reports;					\
      if (!sigsetjmp (env, 1))					\
	{							\
	  __VA_ARGS__;						\
	}							\
      std::printf (name " %d\n", reports - before);		\
    }								\
  while (0)

int
main ()
{
  volatile int big = 40, one = 1, mx = __INT_MAX__, mn = -__INT_MAX__ - 1;
  volatile int m1 = -1, zero = 0;
  volatile int r;
  std::signal (SIGFPE, on_fpe);
  COUNT ("shl", r = shl (one, big));
  COUNT ("add", r = add (mx, one));
  COUNT ("sub", r = sub (mn, one));
  COUNT ("mul", r = mul (mx, mx));
  COUNT ("neg", r = neg (mn));
  COUNT_TRAP ("dvo", r = dvo (mn, m1));
  COUNT_TRAP ("dvz", r = dvz (one, zero));
  COUNT ("ishl", r = one << big);
  COUNT ("iadd", r = mx + one);
  COUNT ("isub", r = mn - one);
  COUNT ("imul", r = mx * mx);
  COUNT ("ineg", r = -mn);
  COUNT_TRAP ("idvo", r = mn / m1);
  COUNT_TRAP ("idvz", r = one % zero);
  (void) r;
  return 0;
}
