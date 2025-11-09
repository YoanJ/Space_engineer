// === [YSI Ship Status v1] ===
// Displays tagged ship resource levels on a single LCD or cockpit surface.

const string TAG_LABEL = "[YSI]";
const string TAG_PREFIX = "[YSI";
const int DEFAULT_SURFACE_INDEX = 0;
const double REFRESH_SECONDS = 5;

IMyTextSurface displaySurface;
IMyTerminalBlock displayBlock;
int displaySurfaceIndex = DEFAULT_SURFACE_INDEX;

List<IMyGasTank> hydrogenTanks = new List<IMyGasTank>();
List<IMyGasTank> oxygenTanks = new List<IMyGasTank>();
List<IMyBatteryBlock> batteries = new List<IMyBatteryBlock>();
List<IMyCargoContainer> cargoContainers = new List<IMyCargoContainer>();
List<IMyTerminalBlock> displayCandidates = new List<IMyTerminalBlock>();

List<string> scratchTags = new List<string>();
List<string> menuOptions = new List<string>();

Dictionary<string, ResourceGroup> tagGroups = new Dictionary<string, ResourceGroup>(StringComparer.OrdinalIgnoreCase);
List<string> tagList = new List<string>();
ResourceGroup totalGroup = new ResourceGroup();

ViewMode currentView = ViewMode.All;
string currentTagKey = null;
int currentTagIndex = -1;
int selectedOptionIndex = 0;

double secondsSinceLast = 0;

enum ViewMode {
    All,
    Tag
}

enum ResourceKind {
    Hydrogen,
    Oxygen,
    Batteries,
    Cargo
}

struct ResourceStats {
    public double Fill;
    public double Current;
    public double Capacity;
    public int Blocks;

    public ResourceStats(double fill, double current, double capacity, int blocks) {
        Fill = fill;
        Current = current;
        Capacity = capacity;
        Blocks = blocks;
    }
}

class ResourceAccumulator {
    public double Current;
    public double Capacity;
    public int Count;

    public void Reset() {
        Current = 0;
        Capacity = 0;
        Count = 0;
    }

    public void Add(double current, double capacity) {
        Current += current;
        Capacity += capacity;
        Count++;
    }

    public ResourceStats ToStats() {
        double fill = Capacity > 0 ? Current / Capacity : 0;
        return new ResourceStats(fill, Current, Capacity, Count);
    }
}

class ResourceGroup {
    public ResourceAccumulator Hydrogen = new ResourceAccumulator();
    public ResourceAccumulator Oxygen = new ResourceAccumulator();
    public ResourceAccumulator Batteries = new ResourceAccumulator();
    public ResourceAccumulator Cargo = new ResourceAccumulator();

    public void Clear() {
        Hydrogen.Reset();
        Oxygen.Reset();
        Batteries.Reset();
        Cargo.Reset();
    }
}

public Program() {
    Runtime.UpdateFrequency = UpdateFrequency.Update100; // ~1.6 s
    RefreshBlocks();
    WriteStatus();
}

public void Main(string argument, UpdateType updateSource) {
    bool commandHandled = false;

    if (!string.IsNullOrWhiteSpace(argument)) {
        string trimmed = argument.Trim();
        string lower = trimmed.ToLowerInvariant();

        if (lower == "refresh") {
            RefreshBlocks();
            WriteStatus();
            return;
        }

        if (lower == "up") {
            MoveSelection(-1);
            commandHandled = true;
        } else if (lower == "down") {
            MoveSelection(1);
            commandHandled = true;
        } else if (lower == "apply") {
            ApplySelection();
            commandHandled = true;
        }
    }

    if (commandHandled) {
        secondsSinceLast = 0;
        WriteStatus();
        return;
    }

    secondsSinceLast += Runtime.TimeSinceLastRun.TotalSeconds;
    if (secondsSinceLast < REFRESH_SECONDS) return;

    secondsSinceLast = 0;
    WriteStatus();
}

void RefreshBlocks() {
    hydrogenTanks.Clear();
    oxygenTanks.Clear();
    batteries.Clear();
    cargoContainers.Clear();

    GridTerminalSystem.GetBlocksOfType(hydrogenTanks, t =>
        t.IsSameConstructAs(Me) && IsTankType(t, "Hydrogen"));

    GridTerminalSystem.GetBlocksOfType(oxygenTanks, t =>
        t.IsSameConstructAs(Me) && IsTankType(t, "Oxygen"));

    GridTerminalSystem.GetBlocksOfType(batteries, b =>
        b.IsSameConstructAs(Me));

    GridTerminalSystem.GetBlocksOfType(cargoContainers, c =>
        c.IsSameConstructAs(Me));

    displaySurface = null;
    displayBlock = null;
    displaySurfaceIndex = DEFAULT_SURFACE_INDEX;
    LocateDisplaySurface();
}

void LocateDisplaySurface() {
    displayCandidates.Clear();
    GridTerminalSystem.GetBlocksOfType(displayCandidates, block =>
        block.IsSameConstructAs(Me) &&
        block is IMyTextSurfaceProvider &&
        HasTag(block.CustomName));

    for (int i = 0; i < displayCandidates.Count; i++) {
        IMyTerminalBlock candidate = displayCandidates[i];
        IMyTextSurfaceProvider provider = candidate as IMyTextSurfaceProvider;
        if (provider == null) continue;

        int index = DEFAULT_SURFACE_INDEX;
        if (!TryGetTagIndex(candidate.CustomName, out index)) {
            index = DEFAULT_SURFACE_INDEX;
        }

        if (index < 0 || index >= provider.SurfaceCount) continue;

        IMyTextSurface surface = provider.GetSurface(index);
        if (surface == null) continue;

        displayBlock = candidate;
        displaySurface = surface;
        displaySurfaceIndex = index;
        ConfigureDisplay(displayBlock, displaySurface);
        break;
    }
}

void ConfigureDisplay(IMyTerminalBlock block, IMyTextSurface surface) {
    surface.ContentType = ContentType.TEXT_AND_IMAGE;
    surface.Font = "Monospace";
    surface.FontSize = GetAdaptiveFontSize(block, surface);
    surface.FontColor = Color.White;
    surface.BackgroundColor = Color.Black;
    surface.Alignment = TextAlignment.LEFT;
    surface.TextPadding = 2f;
}

float GetAdaptiveFontSize(IMyTerminalBlock block, IMyTextSurface surface) {
    var size = surface.SurfaceSize;
    double minDimension = Math.Min(size.X, size.Y);
    if (minDimension <= 0) minDimension = 256;

    double scale = minDimension / 512.0;
    if (scale <= 0) scale = 0.5;

    double baseFont = (block is IMyCockpit || block is IMyShipController) ? 0.85 : 1.2;
    double font = baseFont * scale;

    if (font < 0.55) font = 0.55;
    if (font > 1.4) font = 1.4;

    return (float)font;
}

void WriteStatus() {
    if (!EnsureDisplay()) {
        Echo("No tagged display surface found. Add " + TAG_LABEL + "[:index] to an LCD or cockpit.");
        return;
    }

    RebuildResourceGroups();

    ResourceGroup activeGroup = totalGroup;
    string header = "ALL";

    if (currentView == ViewMode.Tag && currentTagIndex >= 0 && currentTagIndex < tagList.Count) {
        string key = tagList[currentTagIndex];
        ResourceGroup taggedGroup;
        if (tagGroups.TryGetValue(key, out taggedGroup)) {
            activeGroup = taggedGroup;
            header = "GRID [" + key + "]";
            currentTagKey = key;
        } else {
            currentView = ViewMode.All;
            header = "ALL";
            currentTagKey = null;
        }
    } else if (currentView == ViewMode.Tag) {
        currentView = ViewMode.All;
        header = "ALL";
        currentTagKey = null;
    } else {
        currentTagKey = null;
    }

    ResourceStats hydrogen = activeGroup.Hydrogen.ToStats();
    ResourceStats oxygen = activeGroup.Oxygen.ToStats();
    ResourceStats power = activeGroup.Batteries.ToStats();
    ResourceStats cargo = activeGroup.Cargo.ToStats();

    System.Text.StringBuilder sb = new System.Text.StringBuilder();
    sb.AppendLine(header);
    sb.AppendLine("----");
    sb.AppendLine("INFOS");
    sb.AppendLine("----");

    AppendSection(sb, "Hydrogen", hydrogen, "L");
    AppendSection(sb, "Oxygen", oxygen, "L");
    AppendSection(sb, "Batteries", power, "MWh");
    AppendSection(sb, "Cargo", cargo, "L");

    List<string> options = BuildMenuOptions();
    if (options.Count > 0) {
        sb.AppendLine();
        for (int i = 0; i < options.Count; i++) {
            string prefix = i == selectedOptionIndex ? ">" : "-";
            sb.Append(prefix).Append(' ').AppendLine(options[i]);
        }
    }

    displaySurface.WriteText(sb.ToString());
}

bool EnsureDisplay() {
    if (displaySurface != null) return true;
    LocateDisplaySurface();
    return displaySurface != null;
}

void AppendSection(System.Text.StringBuilder sb, string label, ResourceStats stats, string unit) {
    sb.AppendLine(label);

    if (stats.Blocks == 0) {
        sb.AppendLine("  (none detected)");
        sb.AppendLine();
        return;
    }

    sb.AppendFormat("  {0,5:0.0}% {1}\n", stats.Fill * 100, BuildBar(stats.Fill, 24));
    sb.AppendFormat("  {0} / {1} {2}\n", FormatAmount(stats.Current), FormatAmount(stats.Capacity), unit);
    sb.AppendFormat("  {0} blocks\n\n", stats.Blocks);
}

void RebuildResourceGroups() {
    totalGroup.Clear();
    tagGroups.Clear();
    tagList.Clear();

    for (int i = 0; i < hydrogenTanks.Count; i++) {
        IMyGasTank tank = hydrogenTanks[i];
        double capacity = tank.Capacity;
        double current = tank.FilledRatio * capacity;
        ProcessResource(current, capacity, ResourceKind.Hydrogen, tank.CustomName);
    }

    for (int i = 0; i < oxygenTanks.Count; i++) {
        IMyGasTank tank = oxygenTanks[i];
        double capacity = tank.Capacity;
        double current = tank.FilledRatio * capacity;
        ProcessResource(current, capacity, ResourceKind.Oxygen, tank.CustomName);
    }

    for (int i = 0; i < batteries.Count; i++) {
        IMyBatteryBlock battery = batteries[i];
        double current = battery.CurrentStoredPower;
        double capacity = battery.MaxStoredPower;
        ProcessResource(current, capacity, ResourceKind.Batteries, battery.CustomName);
    }

    for (int i = 0; i < cargoContainers.Count; i++) {
        IMyCargoContainer container = cargoContainers[i];
        IMyInventory inventory = container.GetInventory();
        double current = (double)inventory.CurrentVolume;
        double capacity = (double)inventory.MaxVolume;
        ProcessResource(current, capacity, ResourceKind.Cargo, container.CustomName);
    }

    tagList.Sort(StringComparer.OrdinalIgnoreCase);

    if (tagList.Count == 0) {
        currentTagIndex = -1;
        currentTagKey = null;
        if (currentView == ViewMode.Tag) {
            currentView = ViewMode.All;
            selectedOptionIndex = 0;
        }
    } else {
        if (!string.IsNullOrEmpty(currentTagKey)) {
            currentTagIndex = FindTagIndex(currentTagKey);
        }
        if (currentTagIndex < 0) currentTagIndex = 0;
        if (currentTagIndex >= tagList.Count) currentTagIndex = tagList.Count - 1;
        currentTagKey = tagList[currentTagIndex];
    }

    BuildMenuOptions();
}

void ProcessResource(double current, double capacity, ResourceKind kind, string customName) {
    AddToAccumulator(totalGroup, kind, current, capacity);
    AddToTagGroups(customName, kind, current, capacity);
}

void AddToAccumulator(ResourceGroup group, ResourceKind kind, double current, double capacity) {
    ResourceAccumulator accumulator = GetAccumulator(group, kind);
    accumulator.Add(current, capacity);
}

ResourceAccumulator GetAccumulator(ResourceGroup group, ResourceKind kind) {
    switch (kind) {
        case ResourceKind.Hydrogen:
            return group.Hydrogen;
        case ResourceKind.Oxygen:
            return group.Oxygen;
        case ResourceKind.Batteries:
            return group.Batteries;
        case ResourceKind.Cargo:
            return group.Cargo;
        default:
            return group.Hydrogen;
    }
}

void AddToTagGroups(string customName, ResourceKind kind, double current, double capacity) {
    int tagCount = ExtractTags(customName);
    for (int i = 0; i < tagCount; i++) {
        string tag = scratchTags[i];
        ResourceGroup group = GetOrCreateGroup(tag);
        AddToAccumulator(group, kind, current, capacity);
    }
}

ResourceGroup GetOrCreateGroup(string tag) {
    ResourceGroup group;
    if (!tagGroups.TryGetValue(tag, out group)) {
        group = new ResourceGroup();
        tagGroups[tag] = group;
        tagList.Add(tag);
    }
    return group;
}

int ExtractTags(string name) {
    scratchTags.Clear();
    if (string.IsNullOrEmpty(name)) return 0;

    int search = 0;
    while (true) {
        int open = name.IndexOf('[', search);
        if (open < 0) break;
        int close = name.IndexOf(']', open + 1);
        if (close < 0) break;

        string content = name.Substring(open + 1, close - open - 1);
        if (!string.IsNullOrEmpty(content)) {
            int colon = content.IndexOf(':');
            if (colon >= 0) content = content.Substring(0, colon);
            content = content.Trim();
            if (content.Length > 0 && !ContainsTag(content)) {
                scratchTags.Add(content);
            }
        }

        search = close + 1;
    }

    return scratchTags.Count;
}

bool ContainsTag(string tag) {
    for (int i = 0; i < scratchTags.Count; i++) {
        if (string.Equals(scratchTags[i], tag, StringComparison.OrdinalIgnoreCase)) {
            return true;
        }
    }
    return false;
}

List<string> BuildMenuOptions() {
    menuOptions.Clear();

    if (currentView == ViewMode.All) {
        if (tagList.Count > 0) {
            menuOptions.Add("Next");
        }
    } else {
        if (tagList.Count > 0) {
            menuOptions.Add("Next");
            menuOptions.Add("Back");
        }
        menuOptions.Add("Main");
    }

    if (menuOptions.Count == 0) {
        selectedOptionIndex = 0;
    } else {
        if (selectedOptionIndex < 0) selectedOptionIndex = 0;
        if (selectedOptionIndex >= menuOptions.Count) selectedOptionIndex = menuOptions.Count - 1;
    }

    return menuOptions;
}

void MoveSelection(int delta) {
    List<string> options = BuildMenuOptions();
    int count = options.Count;
    if (count == 0) return;

    selectedOptionIndex = (selectedOptionIndex + delta) % count;
    if (selectedOptionIndex < 0) selectedOptionIndex += count;
}

void ApplySelection() {
    List<string> options = BuildMenuOptions();
    if (options.Count == 0) return;

    string choice = options[selectedOptionIndex];

    if (choice == "Next") {
        if (tagList.Count == 0) return;

        if (currentView == ViewMode.All) {
            if (currentTagIndex < 0) currentTagIndex = 0;
            else currentTagIndex = (currentTagIndex + 1) % tagList.Count;
            currentView = ViewMode.Tag;
        } else {
            if (currentTagIndex < 0) currentTagIndex = 0;
            else currentTagIndex = (currentTagIndex + 1) % tagList.Count;
        }
        currentTagKey = tagList[currentTagIndex];
        selectedOptionIndex = 0;
    } else if (choice == "Back") {
        if (tagList.Count == 0) return;

        if (currentTagIndex < 0) currentTagIndex = 0;
        currentTagIndex = (currentTagIndex - 1 + tagList.Count) % tagList.Count;
        currentTagKey = tagList[currentTagIndex];
        selectedOptionIndex = 1;
    } else if (choice == "Main") {
        currentView = ViewMode.All;
        selectedOptionIndex = 0;
    }

    BuildMenuOptions();
}

int FindTagIndex(string tag) {
    for (int i = 0; i < tagList.Count; i++) {
        if (string.Equals(tagList[i], tag, StringComparison.OrdinalIgnoreCase)) {
            return i;
        }
    }
    return -1;
}

string BuildBar(double ratio, int length) {
    if (ratio < 0) ratio = 0;
    if (ratio > 1) ratio = 1;

    int filled = (int)Math.Round(ratio * length);
    if (filled > length) filled = length;
    if (filled < 0) filled = 0;

    return "[" + new string('|', filled) + new string('.', length - filled) + "]";
}

string FormatAmount(double value) {
    double abs = Math.Abs(value);
    string suffix = "";

    if (abs >= 1000000000) {
        value /= 1000000000;
        suffix = "G";
    } else if (abs >= 1000000) {
        value /= 1000000;
        suffix = "M";
    } else if (abs >= 1000) {
        value /= 1000;
        suffix = "k";
    }

    string format = abs >= 100 ? "0" : "0.0";
    return value.ToString(format) + suffix;
}

bool HasTag(string name) {
    int dummy;
    return TryGetTagIndex(name, out dummy);
}

bool TryGetTagIndex(string name, out int index) {
    index = DEFAULT_SURFACE_INDEX;
    if (string.IsNullOrEmpty(name)) return false;

    int start = name.IndexOf(TAG_PREFIX, StringComparison.OrdinalIgnoreCase);
    if (start < 0) return false;

    int end = name.IndexOf(']', start);
    if (end < 0) return false;

    string content = name.Substring(start + 1, end - start - 1);
    if (string.IsNullOrEmpty(content)) return false;

    int colon = content.IndexOf(':');
    string core = colon >= 0 ? content.Substring(0, colon) : content;
    if (!core.Equals("YSI", StringComparison.OrdinalIgnoreCase)) return false;

    if (colon >= 0 && colon + 1 < content.Length) {
        string indexPart = content.Substring(colon + 1);
        int parsed;
        if (int.TryParse(indexPart, out parsed) && parsed >= 0) {
            index = parsed;
        }
    }

    return true;
}

bool ContainsIgnoreCase(string source, string value) {
    if (string.IsNullOrEmpty(source) || string.IsNullOrEmpty(value)) return false;
    return source.IndexOf(value, StringComparison.OrdinalIgnoreCase) >= 0;
}

bool IsTankType(IMyGasTank tank, string keyword) {
    if (ContainsIgnoreCase(tank.BlockDefinition.SubtypeName, keyword)) return true;
    if (ContainsIgnoreCase(tank.DefinitionDisplayNameText, keyword)) return true;

    string info = tank.DetailedInfo;
    if (!string.IsNullOrEmpty(info) && ContainsIgnoreCase(info, keyword)) return true;

    return false;
}
