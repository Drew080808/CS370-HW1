# PROMPTLOG

## Episode: fuzzer sequencing question (pre-M2)
**Context:** Before starting the M2 delete-logic plan, asked the agent for a status summary (what's implemented vs. not) to confirm M1 was actually done and there was no unfinished business blocking delete work.

**Question asked:** Is it standard practice to write fuzzers last, since they won't have anything to fuzz until the API is more complete?

**What came back:** Not "last" specifically — fuzzing earns its keep once there's a mutating surface with nontrivial interactions (insert + delete + find together), since a fuzzer over insert-only mostly duplicates the `rb_validate` assertions already run after every insert in the unit tests. Delete is the highest-risk code (fixup cases, frees), so pairing the fuzzer with M2 concentrates fuzzing effort where it actually catches bugs unit tests won't think to construct.

**My judgment call:** Agreed — keeping the fuzzer stubbed through M1 and building it out alongside delete (per the existing CLAUDE.md milestone split) rather than front-loading a partial insert/find-only fuzzer.
