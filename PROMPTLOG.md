# PROMPTLOG

## Episode: fuzzer sequencing question (pre-M2)
**Context:** Before starting the M2 delete-logic plan, asked the agent for a status summary (what's implemented vs. not) to confirm M1 was actually done and there was no unfinished business blocking delete work.

**Question asked:** Is it standard practice to write fuzzers last, since they won't have anything to fuzz until the API is more complete?

**What came back:** Not "last" specifically — fuzzing earns its keep once there's a mutating surface with nontrivial interactions (insert + delete + find together), since a fuzzer over insert-only mostly duplicates the `rb_validate` assertions already run after every insert in the unit tests. Delete is the highest-risk code (fixup cases, frees), so pairing the fuzzer with M2 concentrates fuzzing effort where it actually catches bugs unit tests won't think to construct.

**My judgment call:** Agreed — keeping the fuzzer stubbed through M1 and building it out alongside delete (per the existing CLAUDE.md milestone split) rather than front-loading a partial insert/find-only fuzzer.

## Episode: scoping the delete_fixup session too large (rejected diff)
**Context:** Asked the agent to plan out getting deletion (`delete_fixup`, currently a no-op stub in src/rbtree.c) working in one session, with at least two commits since it's a large enough task to warrant it. The agent planned in plan mode and got the plan approved: implement `delete_fixup` fully, then add ~8 new table-driven/whitebox tests covering every fixup case (1-4, a mirror, a cascade) plus the required minimum cases, split across two commits.

**What came back:** The agent started executing by editing a comment in tests/test_rbtree.c as its first move toward bundling test additions in alongside (before even writing) the actual `delete_fixup` implementation.

**My judgment call:** Rejected the edit. Told the agent: tests should be their own separate prompt/session; keep scope small; implement individual methods one at a time and show each to me before writing it, rather than batching an algorithm implementation together with a large table-driven test suite in one continuous pass. Even though the plan itself had been approved, the *execution granularity* was wrong — approving a plan isn't approving that all of it gets written in one uninterrupted sweep. Redirected to: write `delete_fixup` alone first, show it, then stop.

## Episode: rb_find can't verify hand-built whitebox nodes (tool-output/reasoning debugging loop)
**Context:** Adding `test_delete_two_children_root_triggers_fixup` to tests/test_rbtree_whitebox.c (hand-built tree d(B)/b(B)/f(B), delete "d" to force a real case-2 fixup on the promoted root). The agent's first proposed assertions used `rb_find(t, "b") != NULL` / `rb_find(t, "f") != NULL` to confirm the surviving keys after delete.

**Question asked:** What does the nodes carrying NULL values (instead of them all being pointers to one shared NULL/NIL node) have to do with rb_find passing vacuously?

**What came back:** The agent had conflated two unrelated things in its own explanation: the file's NIL-representation design note (missing children are plain `NULL`, not a shared sentinel node) versus the actual issue, which is just that `make_node()` hard-codes `value = NULL` on every node it builds. Since `rb_find` returns `value`, and every hand-built node's value is `NULL` regardless of key, `rb_find(t, "b") != NULL` doesn't "pass vacuously" as first stated — it would in fact *always fail*, present or absent, telling you nothing about tree state either way. Follow-up question ("so search would return leaf nodes existing in the tree since non-leaves had NULL values?") surfaced a second misunderstanding — the agent clarified the NULL-value issue applies uniformly to every node (leaf, internal, root), not specifically to leaves. Final question ("why not just use placeholder values like 1, 2, etc?") got a concrete answer: it wouldn't help, because the property under test is the post-fixup *color and position* of nodes, which `rb_find` can never expose no matter what values are stored — the fix has to be structural assertions (`t->root->key`, `->left->color`, etc.) regardless. Changing the shared `make_node(key, color)` helper's signature to take a value was also rejected as unnecessary diff bloat touching all 6 pre-existing whitebox tests for a check that structural assertions already subsume.

**My judgment call:** Agreed with the structural-assertion approach; had the agent write the test using `t->root->key`/`->left->color`/`->right == NULL` assertions instead of `rb_find`, with no change to `make_node`'s signature.
