#!/bin/bash
# verify.sh -- re-measure every reproducer in this directory, against stock
# upstream trunk and against this branch, and report anything that moved.
#
# WHY THIS EXISTS.  A row leaves this directory when *upstream* fixes the bug,
# whoever fixed it -- and nothing else notices that happening.  The xfail watch
# tests in the suite catch a fix landing on *our* branch; they say nothing
# about stock.  Before this script the only way to know was to re-run thirty
# reproducers by hand, which is to say: it was never done, and a row could sit
# here for months after upstream closed it.
#
# The rebase runbook runs this after every rebase, once the branch compiler has
# been rebuilt and the stock nightly refreshed.  Both halves matter -- a stale
# nightly answers last week's question.
#
# USAGE
#   ./verify.sh                 measure both compilers, diff against
#                               verify-expected.txt, exit 1 if anything moved
#   ./verify.sh --record        rewrite verify-expected.txt from what is
#                               measured now (review the git diff!)
#   ./verify.sh GCC-29 GCC-33   just those cases
#   ./verify.sh --stock-only    skip the branch compiler (e.g. before a build)
#   ./verify.sh -v GCC-30       print the diagnostics behind a moved digest
#
# Compilers come from the environment, so an older release can be substituted
# to bisect when upstream fixed something:
#   STOCK_CXX=~/repos/compilers/bin/g++-16.2.0 ./verify.sh --stock-only
#
# THE CLASSIFICATION IS DELIBERATELY THREE-WAY -- ice / error / clean -- and
# not two.  Collapsing "ill-formed" into "clean" (anything that is not an ICE)
# produced two wrong findings during the GCC-7 investigation: a case looked
# immune to a bug when it was really being rejected earlier for an unrelated
# reason.  If you add a mode, keep that distinction.

set -uo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cases=$here/verify-cases.txt
expected=$here/verify-expected.txt

STOCK_CXX=${STOCK_CXX:-$HOME/repos/compilers/bin/g++-trunk}
STOCK_CC=${STOCK_CC:-$HOME/repos/compilers/bin/gcc-trunk}
BRANCH_CXX=${BRANCH_CXX:-$HOME/repos/gnu_gcc-install/bin/g++}
BRANCH_CC=${BRANCH_CC:-$HOME/repos/gnu_gcc-install/bin/gcc}
BRANCH_LIB=${BRANCH_LIB:-$HOME/repos/gnu_gcc-install/lib64}

record=0
stock_only=0
verbose=0
want=()
for arg in "$@"; do
    case $arg in
        --record)     record=1 ;;
        --stock-only) stock_only=1 ;;
        -v|--verbose) verbose=1 ;;
        -h|--help)    sed -n '2,33p' "$0"; exit 0 ;;
        -*)           echo "verify.sh: unknown option $arg" >&2; exit 2 ;;
        *)            want+=("$arg") ;;
    esac
done

[ -r "$cases" ] || { echo "verify.sh: no $cases" >&2; exit 2; }

tmp=$(mktemp -d) || exit 2
trap 'rm -rf "$tmp"' EXIT

_DIAG_RE=':[0-9]+:[0-9]+: (error|warning|note|fatal error|internal compiler error):'

# Normalised diagnostic text for the filtered log $1: the diagnostics only,
# with every path that varies between runs or between checkouts removed.
#
# Without the path scrubbing the digest below would change on every run (GCC
# names its temporaries /tmp/ccXXXXXX.o) and would differ between two clones
# of this repo, which would make the checked-in baseline worthless.
_diag_lines() {
    grep -E "$_DIAG_RE" "$1" \
        | sed -E "s!$here/!!g; s!$tmp/!!g; s!/tmp/cc[A-Za-z0-9]+!TMP!g"
}

# A short fingerprint of the diagnostics: "<count>:<hash>".
#
# THIS IS WHAT MAKES THE ERROR ROWS USEFUL.  Seven of these reproducers are
# rejected by both stock and this branch, but for different reasons -- ours
# because the defect is fixed and the program is genuinely ill-formed, stock's
# because it trips over something else first.  A bare "error -> error" cannot
# tell those apart, so it could never report an upstream fix in those rows,
# which is the one thing this script exists to do.  The digest can.
#
# The cost is sensitivity: an upstream rewording flips the hash with no
# behavioural change.  That is the right trade at this cadence -- a reworded
# diagnostic in one of these areas is itself worth a look, and ./verify.sh
# --verbose prints the lines so a moved digest takes a minute to explain.
_diag_digest() {
    local n h
    n=$(_diag_lines "$1" | wc -l)
    h=$(_diag_lines "$1" | sha1sum | cut -c1-8)
    echo "$n:$h"
}

# Classify one compile/run outcome from its exit code and its output.
#
# The source-echo filter is load-bearing.  GCC quotes the offending source
# line back, and these reproducers have "ICE" and "error:" written in their
# own comments -- so an unfiltered grep reports an ICE for a program that
# merely got a diagnostic on a line whose comment says the word.  Echoed
# lines are the ones GCC renders as "   20 |   ..." or as a bare "      | ^".
_classify() {
    local rc=$1 log=$2 filtered=$tmp/filtered klass
    sed -E '/^ *[0-9]+ \|/d; /^ *\|/d' "$log" > "$filtered"

    if grep -qE 'internal compiler error|Please submit a full bug report' "$filtered"; then
        klass=ice
    elif grep -qE 'undefined reference|ld returned [0-9]+ exit status' "$filtered"; then
        klass=link-error
    elif grep -qE ':[0-9]+:[0-9]+: (error|fatal error):' "$filtered"; then
        klass=error
    elif [ "$rc" -eq 0 ]; then
        # A clean compile can still have said something; a warning appearing or
        # disappearing is a behaviour change worth catching.
        if [ "$(_diag_lines "$filtered" | wc -l)" -gt 0 ]; then
            echo "clean[$(_diag_digest "$filtered")]"
        else
            echo clean
        fi
        [ $verbose -eq 1 ] && _diag_lines "$filtered" | sed 's/^/           /' >&2
        return
    else
        klass="other(rc=$rc)"
    fi

    [ $verbose -eq 1 ] && _diag_lines "$filtered" | sed 's/^/           /' >&2
    echo "$klass[$(_diag_digest "$filtered")]"
}

# Runtime output is part of the measurement for the wrong-code bugs (GCC-10,
# GCC-12, GCC-15): they compile clean and misbehave, so an exit code alone
# would call them fixed.
#
# Three things must be normalised away or the baseline cries wolf on every
# run.  Addresses, obviously.  The source path, or the baseline would differ
# between two clones of this repo.  And any integer of five digits or more:
# GCC-10 demonstrates its bug by printing the garbage the predicate read out
# of the wrong object, which is a different number every execution -- while
# the values these tests care about (2, 7, 19) are all small, so clamping
# long integers costs no signal and buys a stable baseline.
_normalise_output() {
    sed -E 's/0x[0-9a-fA-F]+/0xADDR/g; s![^ ]*/bug-reports/!!g; s/-?[0-9]{5,}/NNNN/g' \
        | tr '\n' ' ' | tr -s ' ' | cut -c1-160 | sed -E 's/ +$//'
}

# Measure case $3 (file $4, mode $5, flags $6) with compiler kind $1
# ("stock" or "branch"); $2 is unused and exists so this stays call-compatible
# with the copy in the Clang fork.
#
# The kind decides whether LD_LIBRARY_PATH is set, and that is not cosmetic.
# The stock wrappers in ../compilers/bin already bake their own rpath; pointing
# them at this branch's lib64 as well would run a stock-compiled binary against
# our libstdc++, which is exactly the mismatch the wrappers exist to prevent.
#
# A mode may carry an "@c" suffix, meaning drive the C compiler rather than the
# C++ one.  Nothing here needs it yet; it is kept so the two forks' copies of
# this script stay identical, and because the C driver is where a
# -fsanitize/link-model bug would have to be measured.
_measure() {
    local kind=$1 file=$4 mode=$5 flags=$6
    local base=${mode%@c} cxx
    if [ "$base" != "$mode" ]; then
        [ "$kind" = stock ] && cxx=$STOCK_CC || cxx=$BRANCH_CC
    else
        [ "$kind" = stock ] && cxx=$STOCK_CXX || cxx=$BRANCH_CXX
    fi

    local log=$tmp/log out=$tmp/a.out rc
    : > "$log"
    case $base in
        syntax)
            $cxx $flags -fsyntax-only "$here/$file" > "$log" 2>&1; rc=$?
            ;;
        compile)
            $cxx $flags -c "$here/$file" -o "$tmp/o.o" > "$log" 2>&1; rc=$?
            ;;
        link)
            $cxx $flags "$here/$file" -o "$out" > "$log" 2>&1; rc=$?
            ;;
        run)
            $cxx $flags "$here/$file" -o "$out" > "$log" 2>&1; rc=$?
            if [ $rc -eq 0 ]; then
                local runout runrc
                if [ "$kind" = branch ]; then
                    runout=$(LD_LIBRARY_PATH=$BRANCH_LIB "$out" 2>&1); runrc=$?
                else
                    runout=$("$out" 2>&1); runrc=$?
                fi
                echo "clean exit:$runrc out:[$(printf '%s' "$runout" | _normalise_output)]"
                return
            fi
            ;;
        *)  echo "bad-mode($mode)"; return ;;
    esac
    _classify "$rc" "$log"
}

_wanted() {
    [ ${#want[@]} -eq 0 ] && return 0
    local w
    for w in "${want[@]}"; do [ "$w" = "$1" ] && return 0; done
    return 1
}

results=$tmp/results
: > "$results"

printf '%-10s %-38s %s\n' ID FILE 'STOCK -> BRANCH'
printf '%-10s %-38s %s\n' -- ---- ---------------

while IFS='|' read -r id file mode flags; do
    case $id in ''|\#*) continue ;; esac
    id=$(echo "$id" | xargs); file=$(echo "$file" | xargs)
    mode=$(echo "$mode" | xargs); flags=$(echo "$flags" | xargs)
    _wanted "$id" || continue

    if [ ! -r "$here/$file" ]; then
        echo "$id|MISSING|MISSING" >> "$results"
        printf '%-10s %-38s %s\n' "$id" "$file" 'MISSING'
        continue
    fi

    s=$(_measure stock '' "$id" "$file" "$mode" "$flags")
    if [ $stock_only -eq 1 ]; then b='(skipped)'
    else b=$(_measure branch '' "$id" "$file" "$mode" "$flags"); fi

    echo "$id|$s|$b" >> "$results"
    printf '%-10s %-38s %s -> %s\n' "$id" "$file" "$s" "$b"
done < "$cases"

echo

if [ $record -eq 1 ]; then
    {
        echo "# Measured behaviour of every reproducer in this directory."
        echo "#"
        echo "# Regenerate with ./verify.sh --record, and READ THE DIFF: a line"
        echo "# that moved in the STOCK column means upstream's behaviour changed,"
        echo "# which is the event that retires a row from README.md.  A line that"
        echo "# moved in the BRANCH column with stock unchanged is a regression"
        echo "# here, or a fix here, and the README Status column should follow."
        echo "#"
        echo "# stock:  $($STOCK_CXX --version | head -1)"
        [ $stock_only -eq 1 ] || echo "# branch: $($BRANCH_CXX --version | head -1)"
        echo "# dated:  $(date +%F)"
        echo "#"
        echo "# id | stock | branch"
        cat "$results"
    } > "$expected"
    echo "recorded $(grep -c '^[^#]' "$results" 2>/dev/null || echo 0) case(s) into $(basename "$expected")"
    exit 0
fi

if [ ! -r "$expected" ]; then
    echo "no $(basename "$expected") yet -- run ./verify.sh --record" >&2
    exit 2
fi

moved=0
while IFS='|' read -r id s b; do
    case $id in ''|\#*) continue ;; esac
    was=$(grep "^$id|" "$expected" | head -1)
    if [ -z "$was" ]; then
        echo "NEW      $id  $s -> $b"; moved=1; continue
    fi
    was_s=$(echo "$was" | cut -d'|' -f2)
    was_b=$(echo "$was" | cut -d'|' -f3)
    if [ "$s" != "$was_s" ]; then
        echo "STOCK    $id  was [$was_s]  now [$s]   <-- upstream moved; re-read README.md's rule for retiring the row"
        moved=1
    fi
    if [ "$b" != "$was_b" ] && [ "$b" != '(skipped)' ]; then
        echo "BRANCH   $id  was [$was_b]  now [$b]"
        moved=1
    fi
done < "$results"

if [ $moved -eq 0 ]; then
    echo "all cases match $(basename "$expected")"
    exit 0
fi
exit 1
