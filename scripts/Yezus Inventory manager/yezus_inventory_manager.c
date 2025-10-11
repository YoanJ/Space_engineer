// === [YIM Base Status v9] ===
// Adds Cargo Containers tracking to [QG] status overview
// Compact layout + ASCII spinner feedback

const string TAG = "[QG]";
const string LCD_NAME = "[QG] LCD [YIM-Main]";
const double REFRESH_SECONDS = 10;

IMyTextPanel lcd;
List<IMyGasTank> h2Tanks = new List<IMyGasTank>();
List<IMyGasTank> o2Tanks = new List<IMyGasTank>();
List<IMyBatteryBlock> batteries = new List<IMyBatteryBlock>();
List<IMyCargoContainer> cargos = new List<IMyCargoContainer>();

double timer = 0;
int frame = 0;
char[] spinner = new char[] { '|', '/', '-', '\\' };

public Program() {
    Runtime.UpdateFrequency = UpdateFrequency.Update100; // ~1.6 s
    RefreshBlocks();
}

void RefreshBlocks() {
    h2Tanks.Clear();
    o2Tanks.Clear();
    batteries.Clear();
    cargos.Clear();

    GridTerminalSystem.GetBlocksOfType(h2Tanks, t =>
        t.CustomName.Contains(TAG) && t.BlockDefinition.SubtypeName.Contains("Hydrogen"));
    GridTerminalSystem.GetBlocksOfType(o2Tanks, t =>
        t.CustomName.Contains(TAG) && t.BlockDefinition.SubtypeName.Contains("Oxygen"));
    GridTerminalSystem.GetBlocksOfType(batteries, b =>
        b.CustomName.Contains(TAG));
    GridTerminalSystem.GetBlocksOfType(cargos, c =>
        c.CustomName.Contains(TAG));

    lcd = GridTerminalSystem.GetBlockWithName(LCD_NAME) as IMyTextPanel;
    if (lcd != null) {
        lcd.ContentType = ContentType.TEXT_AND_IMAGE;
        lcd.Font = "Monospace";
        lcd.FontSize = 0.6f;
        lcd.FontColor = Color.White;
        lcd.BackgroundColor = Color.Black;
        lcd.Alignment = TextAlignment.LEFT;
    }
}

void Main(string argument, UpdateType updateSource) {
    timer += Runtime.TimeSinceLastRun.TotalSeconds;
    if (argument == "refresh") { RefreshBlocks(); timer = 0; }

    if (timer < REFRESH_SECONDS) return;
    timer = 0;

    if (lcd == null) return;

    frame = (frame + 1) % spinner.Length;
    string spin = spinner[frame].ToString();

    string output = "Base " + TAG + " Status Overview  " + spin + "\n=====================\n";
    output += BuildSection("Hydrogen", GetGasData(h2Tanks), "L");
    output += BuildSection("Oxygen", GetGasData(o2Tanks), "L");
    output += BuildSection("Batteries", GetBatteryData(), "MWh");
    output += BuildSection("Containers", GetCargoData(), "L");

    lcd.WriteText(output, false);
}

string BuildSection(string name, double[] data, string unit) {
    double ratio = data[0];
    double cur = data[1];
    double max = data[2];
    int count = (int)data[3];

    string bar = GetMiniBar(ratio, 15);
    string line1 = string.Format("{0}\n", name);
    string line2 = string.Format("- {0,5:0.0}% {1} / {2} {3}\n", ratio * 100, Format(cur), Format(max), unit);
    string line3 = string.Format("- {0}\n", bar);
    string line4 = string.Format("- {0} blocks\n\n", count);

    return line1 + line2 + line3 + line4;
}

string GetMiniBar(double ratio, int len) {
    int filled = (int)(ratio * len);
    return "[" + new string('|', filled) + new string('.', len - filled) + "]";
}

double[] GetGasData(List<IMyGasTank> tanks) {
    double cur = 0, max = 0;
    for (int i = 0; i < tanks.Count; i++) {
        cur += tanks[i].FilledRatio * tanks[i].Capacity;
        max += tanks[i].Capacity;
    }
    double ratio = max > 0 ? cur / max : 0;
    return new double[] { ratio, cur, max, tanks.Count };
}

double[] GetBatteryData() {
    double cur = 0, max = 0;
    for (int i = 0; i < batteries.Count; i++) {
        cur += batteries[i].CurrentStoredPower;
        max += batteries[i].MaxStoredPower;
    }
    double ratio = max > 0 ? cur / max : 0;
    return new double[] { ratio, cur, max, batteries.Count };
}

double[] GetCargoData() {
    double cur = 0, max = 0;
    for (int i = 0; i < cargos.Count; i++) {
        var inv = cargos[i].GetInventory();
        cur += (double)inv.CurrentVolume;
        max += (double)inv.MaxVolume;
    }
    double ratio = max > 0 ? cur / max : 0;
    return new double[] { ratio, cur, max, cargos.Count };
}

string Format(double val) {
    if (val >= 1000000) return (val / 1000000).ToString("0.0M");
    if (val >= 1000) return (val / 1000).ToString("0.0k");
    return val.ToString("0.0");
}
