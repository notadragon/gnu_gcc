// Extra translation unit of refinable-body-ignore-replaceable-template.C:
// the function template f, its assertion ignored, and a caller of f<int>
// compiled with this TU's instantiation in view.

template <typename T>
[[gnu::noinline]] T f (T x)
{
  contract_assert (x > 0);	// ignore
  return x;
}

int use_catch (int y)
{
  try
    {
      return f (y);
    }
  catch (...)
    {
      return -1;
    }
}
