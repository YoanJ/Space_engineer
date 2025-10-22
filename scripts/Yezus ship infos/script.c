// === [YSI Ship Status v1] ===
// Displays tagged ship resource levels on a single LCD or cockpit surface.

const string TAG = "[YSI]";
const int DISPLAY_SURFACE_INDEX = 0;
const double REFRESH_SECONDS = 5;

IMyTextSurface displaySurface;
IMyTerminalBlock displayBlock;

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
        ContainsIgnoreCase(t.BlockDefinition.SubtypeName, "Hydrogen"));

    GridTerminalSystem.GetBlocksOfType(oxygenTanks, t =>
        t.IsSameConstructAs(Me) &&
        ContainsIgnoreCase(t.BlockDefinition.SubtypeName, "Oxygen"));

    GridTerminalSystem.GetBlocksOfType(batteries, b =>
        b.IsSameConstructAs(Me));

    displaySurface = null;
    displayBlock = null;
    LocateDisplaySurface();
}

void LocateDisplaySurface() {
    displayCandidates.Clear();
    GridTerminalSystem.GetBlocksOfType(displayCandidates, block =>
        block.IsSameConstructAs(Me) &&
        HasTag(block.CustomName) &&
        block is IMyTextSurfaceProvider);

    for (int i = 0; i < displayCandidates.Count; i++) {
        IMyTerminalBlock candidate = displayCandidates[i];
        IMyTextSurfaceProvider provider = candidate as IMyTextSurfaceProvider;
        if (provider == null) continue;

        IMyTextSurface surface = null;
        IMyTextPanel panel = candidate as IMyTextPanel;

        if (panel != null) {
            surface = panel;
        } else if (provider.SurfaceCount > DISPLAY_SURFACE_INDEX) {
            surface = provider.GetSurface(DISPLAY_SURFACE_INDEX);
        }

        if (surface == null) continue;

        displayBlock = candidate;
        displaySurface = surface;
        ConfigureDisplay(displaySurface);
        break;
    }
}

void ConfigureDisplay(IMyTextSurface surface) {
    surface.ContentType = ContentType.TEXT_AND_IMAGE;
    surface.Font = "Monospace";
    surface.FontSize = GetAdaptiveFontSize(surface);
    surface.FontColor = Color.White;
    surface.BackgroundColor = Color.Black;
    surface.Alignment = TextAlignment.LEFT;
    surface.TextPadding = 2f;
}

float GetAdaptiveFontSize(IMyTextSurface surface) {
    var size = surface.SurfaceSize;
    double area = size.X * size.Y;
    if (area <= 0) return 0.8f;

    const double referenceArea = 512.0 * 512.0;
    double ratio = area / referenceArea;
    if (ratio <= 0) ratio = 0.1;

    double scale = Math.Sqrt(ratio);
    double font = 1.2 * scale;

    if (font < 0.6) font = 0.6;
    if (font > 1.2) font = 1.2;

    return (float)font;
}

void WriteStatus() {
    if (!EnsureDisplay()) {
        Echo("No tagged display surface found. Add " + TAG + " to an LCD or cockpit.");
        return;
    }

    ResourceStats hydrogen = GetGasStats(hydrogenTanks);
    ResourceStats oxygen = GetGasStats(oxygenTanks);
    ResourceStats power = GetBatteryStats();

    System.Text.StringBuilder sb = new System.Text.StringBuilder();
    sb.Append("Ship ").Append(TAG).AppendLine(" Status");
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
    return name != null && name.Contains(TAG);
}

bool ContainsIgnoreCase(string source, string value) {
    if (string.IsNullOrEmpty(source) || string.IsNullOrEmpty(value)) return false;
    return source.IndexOf(value, StringComparison.OrdinalIgnoreCase) >= 0;
}
