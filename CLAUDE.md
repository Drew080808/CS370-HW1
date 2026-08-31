# rbtree-lab: project rules (CS 370 HW1)

## Context
Red-black tree in C (C23), keyed by C strings (`strcmp` order), values are opaque `void *`.
Milestones: M0 setup, M1 insert/find/foreach/validate, M2 delete (all fixup cases) + fuzzer,
M3 hardening. Full spec is `../HW1_Instructions` (a PDF) — don't re-read it into context;
this file is the operative subset. If something isn't covered here, ask before assuming.

## Commands
- `make test` — build + unit tests + fuzzer (100000 ops)
- `make asan` — AddressSanitizer + UBSan build, runs the test suite
- `make memcheck` — `valgrind --leak-check=full` over the test suite
- A change is DONE only when all three pass. Always actually run them and show real output —
  never assert something passes without running it.

## Hard constraints
- NEVER modify `include/rbtree.h`. It is the frozen, graded contract.
- Check every `malloc` return. A NULL return must leave the tree unchanged and return the
  documented error code; if a later allocation in the same operation fails, free what you
  already allocated on that path.
- NEVER weaken, skip, or delete a test to make the suite pass. If a test looks wrong, stop
  and explain why instead of editing it away.
- Ownership contract: the tree copies each key (and owns the copy). The tree takes ownership
  of `value` ONLY on a successful `rb_insert`; on failure the caller still owns `value`.
  Overwriting an existing key frees the old value via `value_free` before installing the new one.

## Style
- C23. `-Wall -Wextra -Werror` must stay clean. No VLAs.
- Error handling: goto-cleanup pattern for multi-allocation functions.
- Prefer the smallest diff that passes. Do not refactor unrelated code.
- Recursion is allowed this assignment (`rb_destroy`, `rb_foreach`) — the no-recursion
  constraint belongs to the next assignment, not this one.

## Required API (frozen — see include/rbtree.h)
`rb_create`, `rb_insert`, `rb_find`, `rb_delete`, `rb_size`, `rb_foreach`, `rb_validate`, `rb_destroy`.

## rb_validate must check
1. Root is black.
2. No red node has a red child.
3. Every root-to-NIL path crosses the same number of black nodes (black-height).
4. In-order traversal yields strictly increasing keys under `strcmp`.
5. `rb_size` matches the actual node count.

## Delete test coverage (minimum, table-driven)
Red leaf; black leaf with a red sibling; node with two children; deletion of the root;
black node with exactly one (red) child. Each test asserts `rb_validate` and `rb_size` afterward.
Fuzzer must run ≥10^5 random insert/find/delete ops against a reference model, calling
`rb_validate` at least every 100 ops, under both `asan` and `memcheck`.

## Workflow
- For any multi-file or algorithmic change (especially delete fixup): propose a plan in plan
  mode and wait for approval before editing. Enumerate every fixup case and exactly where
  each path frees memory. No code until the plan is approved.
- Implement in small slices, tests first. If a diff comes back touching more than one case/
  file at a time, reject it and ask for a smaller one.
- Before committing a milestone, get an adversarial review from a fresh context (`/code-review`
  or `/clear` + a hostile-reviewer prompt) — hunt for use-after-free, leaked key/value on the
  overwrite path, unchecked allocations, and subtrees `rb_destroy` might miss.
- Commit only from a green state (all three commands above pass); message format `M<n>: <what>`.
  Need ≥8 meaningful commits total for the assignment — no single "final submission" commit.
- New milestone → new session (`/clear`). Use `/compact` mid-task if a debugging thread gets
  noisy, keeping the failing test output and current diff.

## Process deliverables (keep current throughout, not just at the end)
- `PROMPTLOG.md` — 4–6 annotated episodes: a plan I revised, a rejected/oversized diff, a
  tool-output debugging loop, a review finding I triaged. Annotate with what came back and my
  judgment call, not raw transcript dumps.
- `REFLECTION.md` — where the agent was most/least reliable, one bug it introduced that I
  caught, and the biggest C surprise coming from Java.
- Raw Claude Code session transcripts (`.jsonl`) for this project get copied in as-is at
  submission time — work in this project's own dedicated directory so they don't mix with
  unrelated sessions.
