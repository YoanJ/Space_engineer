Displays a quick summary of ship consumables on a tagged LCD or cockpit screen, with per-tag breakdowns.

Scope:
- Track Hydrogen, Oxygen, Battery charge and total Cargo volume across the whole construct
- Show percent, ASCII progress bar and current/max quantity in appropriate units
- Aggregate stats for every `[TAG]` prefix found in the tracked blocks' custom names (only the part before `:` is used)
- A dedicated tag (`[YSI]` or `[YSI:index]`) is required only on the display surface; the script discovers resource blocks automatically

Navigation (`Run` arguments):
- `UP` / `DOWN`: move the cursor through the menu
- `APPLY`: activate the highlighted action (cycle tags, go back, return to main view)
- `refresh`: rescan blocks after renaming or adding hardware

Setup:
1. Rename the programmable block to include `[YSI]` for consistency (optional but helps filtering).
2. Add `[YSI]` or `[YSI:surfaceIndex]` to one LCD panel or cockpit (the index selects which cockpit screen to use, default is 0).
3. Load `script.c` into the programmable block and run once; use the `refresh` argument after renaming blocks.
