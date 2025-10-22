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
List<IMyTerminalBlock> displayCandidates = new List<IMyTerminalBlock>();

double secondsSinceLast = 0;

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

public Program() {
    Runtime.UpdateFrequency = UpdateFrequency.Update100; // ~1.6 s
    RefreshBlocks();
    WriteStatus();
}

public void Main(string argument, UpdateType updateSource) {
    if (!string.IsNullOrWhiteSpace(argument)) {
        string trimmed = argument.Trim();
        if (trimmed.Equals("refresh", StringComparison.OrdinalIgnoreCase)) {
            RefreshBlocks();
            WriteStatus();
            return;
        }
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

    GridTerminalSystem.GetBlocksOfType(hydrogenTanks, t =>
        t.IsSameConstructAs(Me) &&
        IsTankType(t, "Hydrogen"));

    GridTerminalSystem.GetBlocksOfType(oxygenTanks, t =>
        t.IsSameConstructAs(Me) &&
        IsTankType(t, "Oxygen"));

    GridTerminalSystem.GetBlocksOfType(batteries, b =>
        b.IsSameConstructAs(Me));

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

        IMyTextSurface surface = null;
        int index = DEFAULT_SURFACE_INDEX;

        if (!TryGetTagIndex(candidate.CustomName, out index)) {
            index = DEFAULT_SURFACE_INDEX;
        }

        if (index < 0 || index >= provider.SurfaceCount) continue;
        surface = provider.GetSurface(index);

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

    ResourceStats hydrogen = GetGasStats(hydrogenTanks);
    ResourceStats oxygen = GetGasStats(oxygenTanks);
    ResourceStats power = GetBatteryStats();

    System.Text.StringBuilder sb = new System.Text.StringBuilder();
    sb.Append("Ship ").Append(TAG_LABEL).AppendLine(" Status");
    sb.AppendLine("========================");
    AppendSection(sb, "Hydrogen", hydrogen, "L");
    AppendSection(sb, "Oxygen", oxygen, "L");
    AppendSection(sb, "Batteries", power, "MWh");

    displaySurface.WriteText(sb);
}

bool EnsureDisplay() {
    if (displaySurface != null) return true;
    LocateDisplaySurface();
    return displaySurface != null;
}

void AppendSection(System.Text.StringBuilder sb, string label, ResourceStats stats, string unit) {
    sb.AppendLine(label);

    if (stats.Blocks == 0) {
        sb.AppendLine("  (no tagged blocks)");
        sb.AppendLine();
        return;
    }

    sb.AppendFormat("  {0,5:0.0}% {1}\n", stats.Fill * 100, BuildBar(stats.Fill, 24));
    sb.AppendFormat("  {0} / {1} {2}\n\n",
        FormatAmount(stats.Current),
        FormatAmount(stats.Capacity),
        unit);
}

ResourceStats GetGasStats(List<IMyGasTank> tanks) {
    double current = 0;
    double capacity = 0;

    for (int i = 0; i < tanks.Count; i++) {
        IMyGasTank tank = tanks[i];
        double tankCapacity = tank.Capacity;
        capacity += tankCapacity;
        current += tank.FilledRatio * tankCapacity;
    }

    double fill = capacity > 0 ? current / capacity : 0;
    return new ResourceStats(fill, current, capacity, tanks.Count);
}

ResourceStats GetBatteryStats() {
    double current = 0;
    double capacity = 0;

    for (int i = 0; i < batteries.Count; i++) {
        IMyBatteryBlock battery = batteries[i];
        current += battery.CurrentStoredPower;
        capacity += battery.MaxStoredPower;
    }

    double fill = capacity > 0 ? current / capacity : 0;
    return new ResourceStats(fill, current, capacity, batteries.Count);
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

    string content = name.Substring(start + 1, end - start - 1); // e.g. "YSI" or "YSI:2"
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
