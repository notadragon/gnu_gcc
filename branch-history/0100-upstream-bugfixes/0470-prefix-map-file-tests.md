---
id: 0470-prefix-map-file-tests
subject: 'testsuite: __FILE__ prefix-map precedence and -fcanon-prefix-map'
depends: []
regenerates: []
fixes: []
---

## Rationale

Tests only; GCC already behaves as they expect.  For `__FILE__`, a
`-ffile-prefix-map=` is tried before any `-fmacro-prefix-map=` whatever
their order on the command line, and of two `-fmacro-prefix-map=` options
whose old prefixes both match, the last given wins.  Under
`-fcanon-prefix-map` the old prefix, and a file name with a directory, are
compared with `.`, `..` and links resolved, so `./=/mapped` maps a
`#line "././site.exp"` name to `/mapped/site.exp`, where without it (or
after `-fno-canon-prefix-map`) the prefix matches the name as written and
gives `/mapped./site.exp`.  A file name is resolved only when the file
exists, so the `#line` names `site.exp`, which every DejaGnu run has in
its current directory.

## Compile gap

None.  Pure test additions against upstream behaviour; they use no
contracts and nothing this branch adds.

## Contents

- gcc/testsuite/c-c++-common/cpp/fmacro-prefix-map-file-1.c : *
- gcc/testsuite/c-c++-common/cpp/fmacro-prefix-map-file-2.c : *
- gcc/testsuite/c-c++-common/cpp/fmacro-prefix-map-last.c : *
- gcc/testsuite/c-c++-common/cpp/fcanon-prefix-map-1.c : *
- gcc/testsuite/c-c++-common/cpp/fcanon-prefix-map-2.c : *
- gcc/testsuite/c-c++-common/cpp/fcanon-prefix-map-3.c : *
