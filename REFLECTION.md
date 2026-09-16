# REFLECTION

**Where the agent was most reliable:**
The agent was most reliable at producing code, especially translating descriptive, detailed english into actual code and tests. If given a 'delete_fixup' description with what to do in each case, the agent easily produced code that matched my description. The agent was also great at tests - I had a difficult time finding anything to correct when it wrote tests, and after some explicit instructions were added to CLAUDE.md, it became great at running the tests after each change with strict discipline.

**Where it was least reliable:**
The agent struggled more with maintaining an understanding of the current codebase, which improved after I made a habit of including that information in CLAUDE.md at the end of my sessions, but it still struggled sometimes. I wasted some tokens at one point by allowing it to plan a Case 2 Mirror test despite already having a committed test for it. With practice, I'll get better at managing the balance between the benefit of including more of these details in CLAUDE.md versus the downside of more token consumption.

**One bug it introduced that I caught:** 
One bug that the agent introduced which I caught was in tests/fuzz.c, where the agent's first plan for periodic validation used 'if (i % VALIDATE_EVERY == 0)'. Since it's zero-index, this validated after 1, 101, 201, and so on instead of the intended, cleaner 100, 200, 300. This would've made the requirement to 'validate every 100 operations' slightly off. I caught it in review while Claude was still in planning mode and had it correct the plan before writing.

**Biggest C surprise coming from Java:**
I thought I'd be more surprised by memory management in C, but with my C++ experience, I found that memory in C worked very similarly. The thing that surprised me the most about C coming from Java was really the constant use of pointers - it took a lot of getting used to when thinking about whether this pointer would be used to reference an object, or if it was to alter the data at the memory location. Pointers are hugely useful and I'm glad I'm getting used to them.
