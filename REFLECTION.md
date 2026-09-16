# REFLECTION

**Where the agent was most reliable:** Mechanical translation of a fully-specified algorithm
into code and tests. Once a `delete_fixup` case was traced by hand (tree shape -> which
branch/rotation -> final colors), the agent reliably turned that trace into a correct
hand-built whitebox test on the first try, across all four cases and both left/right mirrors,
verified clean under `-Wall -Wextra -Werror`, ASan/UBSan, and valgrind every time.

**Where it was least reliable:** Assumptions about *what already existed* in the codebase,
made without re-checking. It planned a "Case 2 mirror" test on the assumption that the mirror
branch of `delete_fixup` was completely untested -- that turned out to be wrong; an
already-committed test incidentally exercised it. It caught this one itself before writing
anything, but it's a reminder that its claims about existing coverage need verification
(grep/trace), not trust.

**One bug it introduced that I caught:** In `tests/fuzz.c`, the agent's first draft of the
periodic validation check used `if (i % VALIDATE_EVERY == 0)`. Because the loop index is
zero-based, this validates after ops 1, 101, 201, ... instead of after clean 100-op blocks
(100, 200, ...) -- an off-by-one that would've made "validate every 100 ops" not actually mean
what it says. I caught it in review before it was ever written to disk and proposed
`if ((i + 1) % VALIDATE_EVERY == 0)`; the agent agreed and applied the fix immediately.

**Biggest C surprise coming from Java:** TODO -- fill in.
