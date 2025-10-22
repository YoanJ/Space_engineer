Displays a quick summary of the ship consumables on a tagged LCD or cockpit screen.

MVP scope:
- Track Hydrogen, Oxygen and Battery charge across the whole grid
- Show percentage, ASCII progress bar and current/max quantity in appropriate units
- Require a dedicated tag (`[YSI]`) only on the target display surface; resource blocks are auto-discovered
- Use the first tagged LCD (or cockpit surface index 0 if available); future updates can fan out to multiple screens

Setup:
1. Rename the programmable block to include `[YSI]` for consistency (optional but helps filtering).
2. Add `[YSI]` to one LCD panel or cockpit; that surface becomes the display for the MVP.
3. Load `script.c` into the programmable block and run once; use the `refresh` argument after renaming blocks.
