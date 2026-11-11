/* Under -fcanon-prefix-map the old prefix, and a file name with a directory,
   are compared with ".", ".." and links resolved: "./" is the current
   directory, and so is the directory of "././site.exp".
   The name is resolved only when the file exists, so the #line names a
   file every DejaGnu run has in its current directory, site.exp.
   Mirror: edgcpp_compiler tests/tests/contracts/macro_prefix_map/
   gxx_options.sft.cpp.  */
/* { dg-do compile } */
/* { dg-options "-fcanon-prefix-map -fmacro-prefix-map=./=/mapped" } */

#line 100 "././site.exp"
#pragma message "LINE is " __FILE__	/* { dg-message "LINE is /mapped/site\\.exp'" "" { target *-*-* } 100 } */
