/* Without -fcanon-prefix-map an old prefix is compared with the file name
   as written: "./" matches the first two characters of "././site.exp".
   The name is resolved only when the file exists, so the #line names a
   file every DejaGnu run has in its current directory, site.exp.
   Mirror: edgcpp_compiler tests/tests/contracts/macro_prefix_map/
   gxx_options.sft.cpp.  */
/* { dg-do compile } */
/* { dg-options "-fmacro-prefix-map=./=/mapped" } */

#line 100 "././site.exp"
#pragma message "LINE is " __FILE__	/* { dg-message "LINE is /mapped\\./site\\.exp'" "" { target *-*-* } 100 } */
