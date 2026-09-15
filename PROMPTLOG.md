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
