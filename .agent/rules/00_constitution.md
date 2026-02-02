# AGENT CONSTITUTION (IMMUTABLE)

## 1. The Immutable Plan
- You must ALWAYS check `plans/01_master_plan.md` before starting work.
- Do not deviate from the sequence of phases defined in the Master Plan.

## 2. Visual Law (Monochrome Glass)
- **STRICT:** No full black (`#000000`) and No full white (`#FFFFFF`).
- **Backgrounds:** Use Deep Void (`#0A0A0A`) and Carbon (`#121212`).
- **Text:** Use Off-White (`#EDEDED`) for primary text.
- **Glass:** Dark glass surfaces only (`rgba(30, 30, 30, 0.6)` + Blur).

## 3. Safety First
- Memory safety is paramount in this C++ codebase.
- Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers.
- Avoid manual `delete`. Use RAII patterns.

## 4. Verification
- Never mark a task done until it compiles (if applicable to the change).
- For UI changes, verify against the `design_system.md` specs.
