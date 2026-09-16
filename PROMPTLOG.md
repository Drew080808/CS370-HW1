# PROMPTLOG

## Episode: finishing M2 test coverage -- delete_fixup cases, mirrors, and the fuzzer (plan you revised)
**Context:** Asked the agent to begin completing the remaining deletion tests, starting with the first fixup-case test.

**Question asked (fuzzer design):** After all eight fixup-case/mirror tests eventually landed, asked the agent to design tests/fuzz.c (which was still a stub) next: a bounded 500-key reference model, insert/find/delete chosen 1/3 each, rb_size checked every op, rb_validate on a periodic cadence, fixed default seed for reproducibility. Reviewed the full proposed file before it was written and caught an off-by-one: if (i % VALIDATE_EVERY == 0) validates after ops 1, 101, 201, ... (an weirdly early first check just one op in, then a 100-op cadence offset by 1) rather than a clean 100, 200, etc.

**What came back:** The agent agreed immediately and proposed if ((i + 1) % VALIDATE_EVERY == 0) as the fix.

**My judgment** Straightforward correction, accepted.

## Episode: Scoping the delete_fixup session too large (rejected/oversized diff)
**Context:** Asked the agent to begin planning deletion (delete_fixup specifically, at the time a no-op stub in src/rbtree.c) with very high-level logic, with at least two commits since it's a large enough task to warrant it. The agent planned in plan mode and got the plan approved: implement delete_fixup fully, then add ~8 new table-driven/whitebox tests covering every fixup case (1-4, a mirror, a cascade) plus the required minimum cases, split across two commits.

**What came back:** The agent started executing by editing a comment in tests/test_rbtree.c as its first move toward bundling test additions in alongside (before even writing) the actual delete_fixup implementation.

**My judgment** Rejected the edit. Told the agent: tests should be their own separate prompt/session; keep scope small; implement individual methods one at a time and show each to me before writing it, rather than starting an algorithm implementation together with a large table-driven test suite in one continuous pass. Even though the plan itself had been approved, the scale of the first action was wrong — approving a plan isn't approving that all of it gets written in one uninterrupted sweep. Redirected to: write delete_fixup alone first, show it, then stop.

## Episode: rb_find can't verify hand-built whitebox nodes (tool-output debugging loop)
**Context:** Adding test_delete_two_children_root_triggers_fixup to the whitebox tests. The agent's first proposed assertions used rb_find(t, "b") != NULL / rb_find(t, "f") != NULL to confirm the surviving keys. Explaining why that wouldn't work, it brought up the file's NIL-representation design note (missing children are plain NULL, not a shared sentinel node) -- which sounded unrelated to the actual bug and is what confused me.

**Question asked:** What does nodes carrying NULL values (instead of them all pointing to one shared NIL node) have to do with rb_find passing vacuously?

**What came back:** The agent had conflated two unrelated things: the NIL-representation note versus the real issue, which is just that make_node() hard-codes value = NULL on every node it builds. Since rb_find returns value, rb_find(t, "b") != NULL would always fail regardless of tree state, telling you nothing either way. A follow-up prompt I asked ("why not just use placeholder values like 1, 2, etc?") got the real answer: it wouldn't help, because the property under test is the post-fixup *color and position* of nodes, which rb_find can never expose no matter what values are stored -- the fix has to be structural assertions instead.

**My judgment** Agreed with the structural-assertion approach; had the agent write the test using t->root->key/->left->color/->right == NULL assertions instead of rb_find, with no change to make_node's signature.

## Episode: adversarial review catches memcheck running fewer fuzzer ops than required (review finding you triaged)
**Context:**Ran /code-review in a seperate session against the M2 fuzzer commit (4cd4f56). This was the review step that hadn't been run yet for the M2 milestone.

**What came back:** Makefile's memcheck target invoked the fuzzer binary with only 20000 ops, while HW1 requires >=10^5 ops under *both* asan and memcheck. make test/make asan already ran the full 100000; memcheck alone was quietly under-running the fuzzer, likely to keep valgrind's runtime down, but at the cost of violating the spec's stated bar.

**My judgment** Legitimate finding, not a false positive -- accepted immediately. Had the agent bump the memcheck target to 100000 (matching test/asan) and re-verify with make clean && make memcheck.

## Episode: Wrong assumption about mirror-branch coverage
**Context:** Partway through executing the approved M2 test-coverage plan (Case 3, Case 4, and
a mirror-branch test), the plan's step 4 called for a "Case 2 mirror" test, assuming
that the mirror/right-child branch of delete_fixup was untested.

**What came back:** Before writing anything, the agent traced the committed
test_delete_two_children_root_triggers_fixup test and found the assumption was wrong - that
test already incidentally exercised the mirror branch's Case 2, because after the transplant,
x_parent->left was non-NULL, which meant the mirror was fulfilled. It flagged
this to me directly, explained the trace, and proposed substituting a Case 4 mirror test
instead of the originally planned Case 2 mirror.

**My judgment** Accepted the revision without objection - the trace was correct and a
Case 2 mirror test would've been pointless.
