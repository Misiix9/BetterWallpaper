# STANDARD OPERATING PROCEDURES (SOP)

## SOP: Fix Bug
1.  **Reproduce:** Confirm the issue locally (compile/run).
2.  **Fix:** Edit code safely (smart pointers, RAII).
3.  **Verify:** Compile and ensure no regressions.
4.  **Log:** Update `plans/progress_log.md`.

## SOP: New Feature
1.  **Ref:** Logic check `plans/01_master_plan.md` & `design_system.md`.
2.  **Plan:** Update `active_context.md`.
3.  **Headers:** Create interfaces in `.hpp` first.
4.  **Implement:** Write logic in `.cpp`.
5.  **Mark:** Update Master Plan checkbox.

## SOP: UI Change
1.  **Check:** Does it match `02_design_memory.md`?
2.  **Hex:** Are you using `#0A0A0A` instead of Black?
3.  **Anim:** Is it 200ms cubic-bezier?

## SOP: Testing Protocol
1.  **Setup:** One-time: `python3 -m venv .venv && .venv/bin/pip install -r tests/requirements.txt`
2.  **Run:** `.venv/bin/python scripts/hand_of_god.py --mode=chaos`
3.  **verify:** Output must be `SUCCESS`.
4.  **Note:** Move mouse to top-left to EMERGEENCY STOP (X11 only).
5.  **Wayland:** If on Wayland (Hyprland), the script runs in "Blind Mode" (Window Focus + Process Monitoring) as full input injection is restricted.
