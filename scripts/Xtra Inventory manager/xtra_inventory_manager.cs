// === [YIM Hydrogen Monitor v3] ===
// Affiche le niveau global des tanks [QG] sur LCD [YIM-Main]
// Barre de progression bleue dynamique
// Optimisé pour serveurs (rafraîchissement 10 s)

const string tagTank = "[QG]";
const string tagLCD = "[YIM-Main]";
const double refreshSeconds = 10;

IMyTextPanel lcd;
List<IMyGasTank> tanks = new List<IMyGasTank>();
double timer = 0;

public Program() {
    Runtime.UpdateFrequency = UpdateFrequency.Update100; // ~1.6 s
    RefreshBlocks();
}

void RefreshBlocks() {
    tanks.Clear();
    GridTerminalSystem.GetBlocksOfType(tanks, t => t.CustomName.Contains(tagTank));
    lcd = GridTerminalSystem.GetBlockWithName(tagLCD) as IMyTextPanel;
    if (lcd != null) {
        lcd.ContentType = ContentType.TEXT_AND_IMAGE;
        lcd.Font = "Monospace";
        lcd.FontSize = 1.0f;
        lcd.FontColor = new Color(0, 160, 255); // Bleu clair
        lcd.BackgroundColor = new Color(0, 0, 10); // Fond sombre bleu
        lcd.Alignment = TextAlignment.LEFT;
        lcd.WriteText("Initialisation...\n");
    }
}

void Main(string argument, UpdateType updateSource) {
    timer += Runtime.TimeSinceLastRun.TotalSeconds;

    if (argument == "refresh") {
        RefreshBlocks();
        lcd?.WriteText("Blocs rafraîchis\n", false);
        timer = 0;
    }

    if (timer < refreshSeconds) return;
    timer = 0;

    if (lcd == null || tanks.Count == 0) return;

    double totalFilled = 0;
    foreach (var t in tanks) totalFilled += t.FilledRatio;
    double avgFill = totalFilled / tanks.Count;

    int barLength = (int)Math.Max(10, 40 * lcd.FontSize);
    string bar = GetBlueBar(avgFill, barLength);

    string output =
        $"[Hydrogène - {tagTank}]\n\n" +
        $"Tanks trouvés: {tanks.Count}\n" +
        $"Remplissage global: {(avgFill * 100):0.0}%\n" +
        $"{bar}\n\n";

    foreach (var t in tanks)
        output += $"{ShortenName(t.CustomName, 22)}: {(t.FilledRatio * 100):0.0}%\n";

    lcd.WriteText(output, false);
}

string GetBlueBar(double ratio, int length) {
    int filled = (int)(ratio * length);
    string full = new string('█', filled);
    string empty = new string('░', length - filled);
    return $"[{full}{empty}]";
}

string ShortenName(string name, int max) {
    if (name.Length <= max) return name;
    return name.Substring(0, max - 3) + "...";
}
