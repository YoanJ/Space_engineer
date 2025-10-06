# Isy’s Inventory Manager (IIM) — Unified AI‑Optimized Guide

---

## General Information

**Script scope:** Automated **sorting**, **balancing**, **autocrafting**, **refinery handling**, **LCD outputs**, and **command-line toggles** for Space Engineers inventories and production blocks.
**Setup note:** Game must be in **Experimental mode** and **In‑game scripts enabled** in world options.
**Starter flow:** Build **Programmable Block → Edit → Browse Workshop → Isy’s Inventory Manager → Check Code → Remember & Exit**. Containers will sort using defaults.

---

## Basic Setup

1. **Programmable Block:** Load the script and run once.
2. **Defaults:** Type containers are auto‑recognized by **keywords** in block names; sorting begins immediately.
3. **Video/guide:** (optional, external resources may provide walkthroughs).

---

## Core Concepts

### Sorting (Type Containers)

* Items are sorted by **type** into containers tagged via **name keywords** (partial matches count).
* **Default keywords:**

```csharp
// Container type keywords
const string oreContainerKeyword = "Ores";
const string ingotContainerKeyword = "Ingots";
const string componentContainerKeyword = "Components";
const string toolContainerKeyword = "Tools";
const string ammoContainerKeyword = "Ammo";
const string bottleContainerKeyword = "Bottles";
```

* Docked ships/vehicles are emptied unless excluded (see below).

### Exclude Containers from Sorting / Counting

* **Locked from sorting:** Add **`Locked`** to a block’s name *or* extend the list:

```csharp
// Inventories skipped by sorting
string[] lockedContainerKeywords = { "Locked", "Seat", "Control Station" };
```

* **Hidden from counting (autocrafting & inventory LCDs):**

```csharp
// Excluded from item counting
string[] hiddenContainerKeywords = { "Hidden" };
// Treat “Show block in Inventory Screen = off” as hidden?
bool treatNotShownAsHidden = false;
```

### Exclude or Ignore Entire Grids

* **Disable only sorting** on a grid by tagging a **Connector** or **Grid name** with:

```csharp
string noSortingKeyword = "[No Sorting]";   // Sorting off; balancing & specials still active
```

* **Ignore IIM entirely** for a grid by tagging a **Connector** or **Grid name** with:

```csharp
string noIIMKeyword = "[No IIM]";           // Fully ignored by IIM
```

---

## Container Priorities & Balancing

### Fill Priorities

* Add **`[P#]`** (1..2.1B) to container names; **lower # = higher priority**.
* **`[PMax]`** = absolute first; **`[PMin]`** = absolute last.
* Special containers still outrank normal type containers; priority tokens apply to them too.

**Examples:**

```
Small Cargo Container 1 Ores [P1]
Small Cargo Container [P13] Ingots
[P5] Cargo Container Components
Cargo Container Bottles [PMax]
```

### Equalize Type Containers (optional)

```csharp
bool balanceTypeContainers = false; // Instruction heavy; disables type‑container priority effects
```

---

## Automatic Container Assignment

Assign/unassign type containers on demand (e.g., when full/empty). Priorities like **`[P1]`** prevent auto‑unassignment.

```csharp
// Master
bool autoContainerAssignment = true;

// Assign new containers automatically
bool assignNewContainers = true;      
bool assignOres = true;
bool assignIngots = true;
bool assignComponents = true;
bool assignTools = true;
bool assignAmmo = true;
bool assignBottles = true;

// Unassign empty containers automatically (keeps at least one per type)
bool unassignEmptyContainers = true;  
bool unassignOres = true;
bool unassignIngots = true;
bool unassignComponents = true;
bool unassignTools = true;
bool unassignAmmo = true;
bool unassignBottles = true;

// Grouping of assigned types
bool oresIngotsInOne = true;          // Ores+Ingots as one pool
bool toolsAmmoBottlesInOne = true;    // Tools+Ammo+Bottles as one pool
```

---

## Bottles, Fill Levels & Internal Sorting

### Bottle Refilling

* Move incoming **bottles** to tanks or O2/H2 generators for refilling, then to **Bottles** container.
* Scripts can’t read bottle fill levels; only bottles **not** already in the bottle container are moved.

```csharp
bool fillBottles = true;
```

### Fill Level & No‑Conveyor Tagging

```csharp
bool showFillLevel = true;   // Show % in managed container names
bool showNoConveyorTag = false; // Append [No Conveyor] when disconnected (requires connectionCheck)
```

### Internal Inventory Sorting (Per‑Inventory)

* Sort items **inside an inventory** by **Amount/Name/Type**; disabled by default (MP desync risk).
* **Global:**

```csharp
bool enableInternalSorting = false; // Use with caution (server load)
string sortingPattern = "Na";      // Quantifier: A,N,T,X + Direction: a,d (e.g., "Ad", "Td")
```

* **Per‑block name:** append **`(sort:PATTERN)`**.

  * Examples: `Small Cargo Container (sort:Aa)`, `(sort:Td)`, `(sort:Xa)`.

---

## Special Loadout Containers

* Add **`Special`** to a container name → it holds **user‑defined items** and is **not drained** by autosorting.
* Still participates in balancing/autocrafting.
* Configure **per‑item mode** in the container’s **Custom Data**:

  * **Normal** `item=100` (store target, remove excess)
  * **Minimum** `item=100M` (keep ≥ target, ignore excess)
  * **Limiter** `item=100L` (don’t store; remove overage)
  * **All** `item=All` (store all of subtype)

```csharp
const string specialContainerKeyword = "Special";
bool allowSpecialSteal = true; // May pull from lower‑priority Specials
```

---

## Autocrafting (Assemblers & Survival Kits)

### Essentials

* Build **LCD** named with **`Autocrafting`** (multi‑LCD supported: `LCD Autocrafting 1`, `... 2`).
* LCD shows **autocraftable items** with **Current | Wanted**; edit numbers to set targets.
* Assemblers are used **in parallel**; queue is **sorted by need**.

```csharp
// Core switches
bool enableAutocrafting = true;
bool enableAutodisassembling = false; // Disassembles above wanted

// LCD keyword
string autocraftingKeyword = "Autocrafting";

// Per‑assembler behavior
string assembleKeyword = "!assemble-only";
string disassembleKeyword = "!disassemble-only";

// Queue margins (percent of wanted)
double assembleMargin = 0;      // e.g., 5 → craft if < 95/100

double disassembleMargin = 0;   // e.g., 10 → disassemble if > 1100/1000

// UI
bool headerOnEveryScreen = false;
bool sortAssemblerQueue = true;

// Survival kits
bool enableBasicIngotCrafting = true; // Disabled automatically if refineries exist
bool disableBasicAutocrafting = true; // Use kits even if assemblers exist? set false

// Manual assembler (excluded from auto)
// Example name: "Assembler 1 !manual"
```

### Autocrafting Modifiers (Per‑Item)

* Set on **Wanted** column to alter handling:

  * **`A`** Assemble only
  * **`D`** Disassemble only
  * **`P`** Priority craft
  * **`H`** Hide but **manage** (moves to LCD Custom Data)
  * **`I`** Hide and **ignore** (don’t manage)
  * **`Y#`** Yield (one craft → **#** items)

**Examples:**

```
SteelPlates   4123 < 5000A   // assemble only
SteelPlates   4123 < 5000D   // disassemble only
SteelPlates   4123 < 5000P   // prioritize
SteelPlates   4123 < 5000H   // hide & manage via Custom Data
SteelPlates   4123 < 5000I   // hide & ignore
Algaesample   4123 < 5000Y50 // per‑craft yield = 50
```

Modifiers can be **combined** (e.g., `AP`, `AD`).

### Unknown/Modded Items (Learning Blueprints)

* Show unlearned items on LCD (optional):

```csharp
bool showUnlearnedItems = false; // Or run script with `showUnlearnedItems` argument
```

* **Learning assemblers:** rename an assembler to one of:

  * **`!learn`** — learns one queued item, then tag removed.
  * **`!learnMany`** — learns each queued stack (queue **one stack at a time**).
* Queue ~**100** crafts; the script removes queue when learned. The **[No BP!]** tag disappears once known.

### Auto Disassembling

* After configuring **Wanted** amounts, set:

```csharp
bool enableAutodisassembling = false; // true = disassemble excess automatically
```

> ⚠️ **Warning:** Enable only **after** setting wanted amounts; otherwise items may be immediately disassembled.

### Assembler Cleanup

```csharp
bool enableAssemblerCleanup = true; // Return contents to cargo when idle/too full
```

---

## Refinery Handling

* Splits **ores evenly** across refineries; queue sorted by **most needed ingots**.
* Refineries with **`!manual`** are excluded.
* Mods with **multi‑output ores** are supported non‑intelligently (processed but not demand‑optimized).

```csharp
bool enableOreBalancing = true;
bool enableScriptRefineryFilling = true; // Move in needed ores; free space as needed
bool sortRefiningQueue = true;

// Fixed ore priorities (refined first, top→bottom). Remove // to enable, order matters
List<string> fixedRefiningList = new List<string> {
    "Stone",
    //"Iron",
    //"Nickel",
    //"Cobalt",
    //"Silicon",
    //"Uranium",
    //"Silver",
    //"Gold",
    //"Platinum",
    //"Magnesium",
    //"Scrap",
};
```

---

## Ice & Uranium Balancing

### Ice (O2/H2 Generators)

* Balance **ice** across all generators; leaves room for bottles (default **1**).
* Generators with **`!manual`** are excluded.

```csharp
bool enableIceBalancing = true;
int spaceForBottles = 1; // Leave capacity for N bottles
```

> Note: O2/H2 generators **auto‑pull** below ~30% inventory. To avoid: disable **Use Conveyor**.

### Uranium (Reactors)

* Balance **uranium** across all reactors; conveyors on reactors are turned **off** to stop pulling.
* Reactors with **`!manual`** are excluded.

```csharp
bool enableUraniumBalancing = true;
double uraniumAmountLargeGrid = 100; // per large reactor
double uraniumAmountSmallGrid = 25;  // per small reactor
```

---

## LCD Outputs (Keywords & Options)

Add a keyword to any LCD/block name; it will be normalized to **`[IsyLCD]`** and configured via **Custom Data**. Use **`@#`** prefix to target a specific **screen index** (e.g., `@0`, `@1`).

**Main panel:** `IIM-main` — fill levels, managed blocks, last actions (toggle sections via Custom Data).
**Warnings:** `IIM-warnings` — problems and alerts.
**Actions:** `IIM-actions` — recent transfers/assignments/queue sorts.

```csharp
bool showTimeStamp = true;
int  maxEntries   = 30;
string timeFormat = "HH:mm:ss";
```

**Performance:** `IIM-performance` — PB terminal/perf output.

### Inventory Panels (`IIM-inventory`)

Define content in **Custom Data** (one entry **per line**). Options:

**1a. Item type:**

```
Ingot
Component
Ore
AmmoMagazine
OxygenContainerObject
GasContainerObject
PhysicalGunObject
```

**1b. Item subtype:**

```
SteelPlate
SmallTube
Computer
```

**1c. Type/Subtype combo:**

```
Ingot/Gold
Ore/Stone
Ingot/Stone
```

**1d. Partial name match:**

```
Plate
Welder
Bottle
```

**1e. Echo lines (v2.8.0+):**

```
Echo My Text
EchoC My Text
EchoR My Text
Echo        // blank line
```

**1f. Regex (v2.9.4+):**

```
.
(Steel)|(Motor)
Item$
```

**2. Optional max bar value:** add a number:

```
Ingot 100000
```

`0` shows **single line** (no bar/percentage):

```
Ingot 0
```

**3. Per‑entry modifiers:** `noHeading`, `singleLine`, `noBar`, `noScroll`, `hideEmpty`, `hideType`

**Example:**

```
@0 IIM-inventory
Ingot 100000 noBar
Ore hideEmpty
Echo
EchoC Amount of Steel Plates:
Echo
SteelPlate 50000 noHeading noBar
```

### Multi‑Panel Inventory Screens

Group multiple LCDs to span content:

```
IIM-inventory:GROUP#   // in each panel’s Custom Data (and keyword in name)
```

* **GROUP** = your label; **#** = panel index in row.
* First panel’s font & settings propagate to the group.

---

## Command‑Line Arguments (Run PB)

**Syntax:** `variable true|false|toggle`

* Omit the second token to **toggle** the current state.
* Changes are **temporary** (reset on recompile/world load).

### General Functions (no extra args)

```
reset           // Delete stored itemIds, blueprintIds, and empty Special data (recompile afterwards)
findBlueprints  // Attempt to learn all blueprints (can generate large logs)
```

### Container‑Related Toggles

```
balanceTypeContainers
showFillLevel
autoContainerAssignment
assignNewContainers
unassignEmptyContainers

oresIngotsInOne
toolsAmmoBottlesInOne
fillBottles
allowSpecialSteal
enableInternalSorting
protectTypeContainers   // Protect type containers on a grid also running IIM when docking
```

### Autocrafting‑Related Toggles

```
enableAutocrafting
enableAutodisassembling
showUnlearnedItems
useDockedAssemblers
headerOnEveryScreen
sortAssemblerQueue
enableAssemblerCleanup
enableBasicIngotCrafting
disableBasicAutocrafting
```

### Refining‑Related Toggles

```
enableOreBalancing
enableScriptRefineryFilling
sortRefiningQueue
useDockedRefineries
```

### Ice‑Balancing Toggles

```
enableIceBalancing
fillOfflineGenerators
```

### Uranium‑Balancing Toggles

```
enableUraniumBalancing
fillOfflineReactors
```

### Miscellaneous Toggles

```
useDynamicScriptSpeed
excludeWelders
excludeGrinders
excludeDrills
connectionCheck
showNoConveyorTag
```

---

## Practical Use Cases

* **Starter base:** Default type containers (`Ores`, `Ingots`, `Components`, `Tools`, `Ammo`, `Bottles`) + **Autocrafting LCD** → automated logistics from day one.
* **Mobile miners:** Tag miner connector **`[No Sorting]`** during ops to prevent emptying; remove tag when docking to unload.
* **Defense readiness:** Configure **Special** container with **Minimum** mode for turret ammo (e.g., `NATO_25x184mm=800M`).
* **Production tuning:** Use **`!assemble-only`** for dedicated assembly lines; apply **`P`** modifier to prioritize steel plates.
* **Refinery throughput:** Enable **ore balancing** and **script filling**; set **Stone** first in `fixedRefiningList` to keep gravel flowing.
* **Server safety:** Leave **internal sorting** off; prefer **inventory panels** for visibility.

---

## Notes & Tips

* Partial name matching means **keywords must be unique** in your block names.
* Containers with priorities **won’t be auto‑unassigned**.
* For exact LCD rendering, ensure **one definition per line** and set font sizes consistently (multi‑panel groups inherit first panel settings).
* When enabling **auto‑disassembling**, **double‑check wanted amounts** to avoid unwanted conversion to ingots.

---

## Glossary (Quick Reference)

* **Type Containers:** Named by keywords (Ores/Ingots/Components/Tools/Ammo/Bottles).
* **Special Containers:** Name contains **Special**; per‑item modes: Normal/Minimum/Limiter/All.
* **Priority Tokens:** `[P#]`, `[PMax]`, `[PMin]`.
* **Grid Exclusions:** `[No Sorting]`, `[No IIM]`.
* **Assembler Tags:** `!manual`, `!assemble-only`, `!disassemble-only`, `!learn`, `!learnMany`.
* **LCD Keywords:** `IIM-main`, `IIM-warnings`, `IIM-actions`, `IIM-performance`, `IIM-inventory`.
