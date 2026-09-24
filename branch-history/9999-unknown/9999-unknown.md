---
id: 9999-unknown
subject: 'UNCATEGORIZED -- blocks generation'
depends: []
regenerates: []
fixes: []
---

## Rationale

Not a commit.  This is the holding pen for any part of the branch diff that
has not yet been attributed to a real commit, and it exists so that
"unclassified" is a state the tooling can see rather than an omission nobody
notices.

Its `## Contents` is **empty, and must stay empty**: `check` refuses to
generate the branch while anything is parked here.  Err toward this
bucket when classifying -- a wrong attribution produces a
commit that looks reviewed and is not, which is far more expensive than an
admitted gap.

If content ever reappears here, that is the intended signal: new work landed
on the working branch without a mapping entry, or a rebase moved code out
from under a selector.  Either way the answer is to classify it, not to
widen a selector until the message goes away.

## Compile gap

Not applicable -- this never becomes a commit.

## Contents

