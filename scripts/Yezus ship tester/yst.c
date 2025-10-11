// === [YST Ship Load Overview v3.8 - Titles, pages, detail nav] ===
// Works on cockpit screens tagged [YST:index] or panels [YST]
// Accurate thrust & load ratios, multi-page LCD, 0g compatible

List<IMyThrust> thrusters = new List<IMyThrust>();
List<IMyShipController> controllers = new List<IMyShipController>();
List<IMyTerminalBlock> containers = new List<IMyTerminalBlock>();
List<IMyCargoContainer> cargoContainers = new List<IMyCargoContainer>();
List<IMyGasTank> gasTanks = new List<IMyGasTank>();
IMyTextSurface surface; // can be cockpit LCD or panel
IMyTextPanel lcd;

public Program() {
    Runtime.UpdateFrequency = UpdateFrequency.None;
    RefreshBlocks();
    if (string.IsNullOrEmpty(Storage)) Storage = "mode=overview;scenario=comp;slice=25;cursor=0";
}

void RefreshBlocks() {
    surface = null;
    lcd = null;

    GridTerminalSystem.GetBlocksOfType(thrusters, t => t.CubeGrid == Me.CubeGrid);
    GridTerminalSystem.GetBlocksOfType(controllers, c => c.CubeGrid == Me.CubeGrid);
    GridTerminalSystem.GetBlocksOfType(containers, c =>
        c.CubeGrid == Me.CubeGrid && (c.HasInventory && !(c is IMyGasTank)));
    GridTerminalSystem.GetBlocksOfType(cargoContainers, c => c.CubeGrid == Me.CubeGrid);
    GridTerminalSystem.GetBlocksOfType(gasTanks, t => t.CubeGrid == Me.CubeGrid);

    // 1) Priorite: LCD panel avec [YST]
    List<IMyTextPanel> panels = new List<IMyTextPanel>();
    GridTerminalSystem.GetBlocksOfType(panels, p => p.CubeGrid == Me.CubeGrid && p.CustomName.Contains("[YST]"));
    if (panels.Count > 0) {
        lcd = panels[0];
        surface = lcd as IMyTextSurface;
        return;
    }

    // 2) Sinon, cockpit tagge [YST:index]
    List<IMyCockpit> cockpits = new List<IMyCockpit>();
    GridTerminalSystem.GetBlocksOfType(cockpits, c => c.CubeGrid == Me.CubeGrid && c.CustomName.Contains("[YST:"));
    foreach (var c in cockpits) {
        var sp = c as IMyTextSurfaceProvider;
        if (sp == null || sp.SurfaceCount == 0) continue;
        string name = c.CustomName;
        int start = name.IndexOf("[YST:") + 5;
        int end = name.IndexOf("]", start);
        if (start < 5 || end < 0) continue;
        string numStr = name.Substring(start, end - start);
        int index;
        if (!int.TryParse(numStr, out index)) index = 0;
        if (index < 0 || index >= sp.SurfaceCount) index = 0;
        surface = sp.GetSurface(index);
        return;
    }
}

public void Main(string argument, UpdateType updateSource) {
    if (surface == null) {
        RefreshBlocks();
        if (surface == null) {
            Echo("No [YST] display found.");
            return; 
        }
    }

    surface.ContentType = ContentType.TEXT_AND_IMAGE;
    surface.Font = "Monospace";
    surface.FontSize = 0.7f;
    surface.Alignment = TextAlignment.LEFT;
    surface.WriteText("", false);

    if (controllers.Count == 0) {
        surface.WriteText("No cockpit/RC found.", false);
        return;
    }

    IMyShipController ctrl = GetMainController();
    MatrixD refMatrix = ctrl.WorldMatrix;
    double shipMass = ctrl.CalculateShipMass().PhysicalMass;

    // === Directional thrust relative to cockpit ===
    double up=0,down=0,left=0,right=0,forward=0,backward=0;
    foreach (var t in thrusters) {
        double thrustKg = t.MaxEffectiveThrust * 0.1019716213;
        Vector3D thrDir = -t.WorldMatrix.Forward; // thrust direction
        Vector3D local = Vector3D.TransformNormal(thrDir, MatrixD.Transpose(refMatrix));

        if (local.Y > 0.9) up += thrustKg;
        else if (local.Y < -0.9) down += thrustKg;
        else if (local.Z < -0.9) forward += thrustKg;   // cockpit -Z is Forward
        else if (local.Z > 0.9) backward += thrustKg;  // cockpit +Z is Backward
        else if (local.X > 0.9) right += thrustKg;
        else if (local.X < -0.9) left += thrustKg;
    }

    // === Cargo ===
    double totalVol=0,usedVol=0;
    for (int i=0;i<containers.Count;i++)
        for (int inv=0;inv<containers[i].InventoryCount;inv++) {
            var invRef=containers[i].GetInventory(inv);
            totalVol+=(double)invRef.MaxVolume;
            usedVol+=(double)invRef.CurrentVolume;
        }
    double totalL=totalVol*1000, usedL=usedVol*1000;
    double fill=(totalL>0)?(usedL/totalL*100):0;

    // === Densités ===
    double compD=0.7, oreD=2.5, iceD=0.9;
    double compMass=totalL*compD, oreMass=totalL*oreD, iceMass=totalL*iceD;

    // === Combo Up + Fw (45°) ===
    double upForward=(up*0.707)+(forward*0.707);

    // === New UI state machine (modes + cursor) ===
    string mode = "overview", scenario = "empty"; int slice = 25; int cursor = 0; int shipSlice = 0;
    if (!string.IsNullOrEmpty(Storage)) {
        var parts = Storage.Split(';');
        for (int i=0;i<parts.Length;i++) {
            string p = parts[i];
            if (p.StartsWith("mode=")) mode = p.Substring(5);
            else if (p.StartsWith("scenario=")) scenario = p.Substring(9);
            else if (p.StartsWith("slice=")) int.TryParse(p.Substring(6), out slice);
            else if (p.StartsWith("cursor=")) int.TryParse(p.Substring(7), out cursor);
            else if (p.StartsWith("shipslice=")) int.TryParse(p.Substring(10), out shipSlice);
        }
    }

    string arg = (argument ?? "").Trim().ToLower();
    if (arg == "comp" || arg == "ore" || arg == "ice") { mode = "scenario_overview"; scenario = arg; cursor=0; }
    if (arg == "empty") { mode = "thrust_overview"; cursor=0; }
    if (arg == "ship") { mode = "ship_overview"; cursor=0; shipSlice=0; }
    if (arg == "menu") { mode = "overview"; cursor=0; }

    // cursor bounds computed by helper outside Main

    if (arg == "up") { if (cursor>0) cursor--; }
    else if (arg == "down") { int max=MaxCursorMode(mode); if (cursor<max) cursor++; }
    else if (arg == "back") {
        if (mode=="thrust_overview"||mode=="overview") { mode = "overview"; cursor=0; }
        else if (mode=="thrust_detail") { mode = "thrust_overview"; cursor=0; }
        else if (mode=="scenario_overview") { mode = "overview"; cursor=0; }
        else if (mode=="scenario_detail") { mode = "scenario_overview"; cursor=0; }
        else if (mode=="scenario_slice") { mode = "scenario_detail"; cursor=0; }
        else if (mode=="ship_overview") { mode = "overview"; cursor=0; }
    }
    else if (arg == "apply") {
        if (mode=="overview") {
            if (cursor==0) { mode = "thrust_overview"; cursor=0; }
            else if (cursor==1) { mode = "scenario_overview"; if (string.IsNullOrEmpty(scenario)) scenario="comp"; cursor=0; }
            else if (cursor==2) { mode = "ship_overview"; cursor=0; shipSlice=0; }
        } else if (mode=="thrust_overview") {
            if (cursor==0) { mode = "thrust_detail"; cursor=0; }
            else if (cursor==1) { mode = "overview"; cursor=0; }
        } else if (mode=="thrust_detail") {
            mode = "thrust_overview"; cursor=0;
        } else if (mode=="scenario_overview") {
            if (cursor==0) {
                string[] order = new[]{"comp","ore","ice"};
                int idx=0; for(int i=0;i<order.Length;i++){ if(order[i]==scenario){ idx=i; break; } }
                scenario = order[(idx+1)%order.Length];
            } else if (cursor==1) { mode = "scenario_detail"; slice=25; cursor=0; }
            else if (cursor==2) { mode = "overview"; cursor=0; }
        } else if (mode=="scenario_detail") {
            if (cursor==0) { mode = "scenario_slice"; slice=25; cursor=0; }
            else if (cursor==1) { mode = "scenario_overview"; cursor=0; }
        } else if (mode=="scenario_slice") {
            if (cursor==0) { slice = (slice==25)?50:(slice==50)?75:(slice==75)?100:25; }
            else if (cursor==1) { mode = "scenario_detail"; cursor=0; }
        } else if (mode=="ship_overview") {
            if (cursor==0) { shipSlice++; }
            else if (cursor==1) { mode = "overview"; cursor=0; }
        }
    }

    // Save
    Storage = "mode="+mode+";scenario="+scenario+";slice="+slice.ToString()+";cursor="+cursor.ToString()+";shipslice="+shipSlice.ToString();

    // Render
    if (mode=="overview") {
        Title("Overview", 1, 1);
        WriteLine("Mass: " + Fm(shipMass));
        Vector3D gVec = ctrl.GetNaturalGravity();
        double g = gVec.Length();
        WriteLine("Gravity: " + g.ToString("0.00") + " m/s^2 (" + (g/9.81).ToString("0.00") + " g)");
        WriteLine("Cargo: " + Fv(usedL) + "/" + Fv(totalL) + " (" + fill.ToString("0.0") + "%)");
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Thrust overview");
        WriteLine((cursor==1?"> ":"  ") + "Scenarios");
        WriteLine((cursor==2?"> ":"  ") + "Ship overview");
        WriteFooterText("up/down, apply, menu");
        return;
    }

    if (mode=="ship_overview") {
        Title("Ship overview", 1, 1);
        int maxLines = ComputeMaxLines(surface);
        int reserve = 4; // footer + options spacing
        int avail = Math.Max(8, maxLines - reserve);

        var sections = BuildShipSections(refMatrix);
        // Slice packing by full sections
        int start = 0; int end = 0; int current = 0;
        while (true) {
            int lines = 0; end = start;
            while (end < sections.Count && lines + sections[end].Count <= avail) { lines += sections[end].Count; end++; }
            if (current == shipSlice) break;
            current++;
            if (end >= sections.Count) { shipSlice = 0; start = 0; current = 0; continue; }
            start = end;
        }
        // Render this slice
        for (int i=start;i<end;i++) {
            var block = sections[i];
            for (int j=0;j<block.Count;j++) WriteLine(block[j]);
            if (i<end-1) WriteLine("");
        }
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Next slice");
        WriteLine((cursor==1?"> ":"  ") + "Back");
        WriteFooterText("apply to navigate");
        return;
    }

    if (mode=="thrust_overview") {
        Title("Thrust overview", 1, 1);
        WriteLine("Empty capacity at 1g:");
        RenderAxisCapacity("UP  ", up, shipMass);
        RenderAxisCapacity("DOWN", down, shipMass);
        RenderAxisCapacity("LEFT", left, shipMass);
        RenderAxisCapacity("RIGHT", right, shipMass);
        RenderAxisCapacity("FWD ", forward, shipMass);
        RenderAxisCapacity("BCK ", backward, shipMass);
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Details");
        WriteLine((cursor==1?"> ":"  ") + "Back");
        WriteFooterText("up/down, apply, back");
        return;
    }

    if (mode=="thrust_detail") {
        Title("Thrust details", 1, 1);
        WriteLine("EMPTY CAPACITY AT 1G:");
        WriteLine("MASS: " + Fm(shipMass));
        WriteLine("");
        WriteLine("UP   | " + Fm(up));
        WriteLine("DOWN | " + Fm(down));
        WriteLine("LEFT | " + Fm(left));
        WriteLine("RIGHT| " + Fm(right));
        WriteLine("FWD  | " + Fm(forward));
        WriteLine("BCK  | " + Fm(backward));
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Back");
        WriteFooterText("apply = back");
        return;
    }

    if (mode=="scenario_overview") {
        Title(CapFirst(scenario)+" overview", 1, 1);
        WriteLine("Fill / Thrusters   25% | 50%");
        WriteLine("                   75% | 100%");
        double sMass = ScenarioMass(scenario, compMass, oreMass, iceMass);
        RenderScenarioRow("Up ", sMass, shipMass, up);
        RenderScenarioRow("Fw ", sMass, shipMass, forward);
        RenderScenarioRow("U+F", sMass, shipMass, upForward);
        // Simple hydro estimation to exit atmosphere
        double earthPct = EstimateHydroPercent(shipMass + sMass, refMatrix, 50000.0); // ~50 km
        double moonPct  = EstimateHydroPercent(shipMass + sMass, refMatrix, 15000.0); // ~15 km
        WriteLine("");
        WriteLine("Hydro to leave:");
        WriteLine("  Earth: " + (earthPct>=0? earthPct.ToString("0.0")+"%" : "N/A"));
        WriteLine("  Moon : " + (moonPct>=0? moonPct.ToString("0.0")+"%" : "N/A"));
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Next scenario");
        WriteLine((cursor==1?"> ":"  ") + "Details");
        WriteLine((cursor==2?"> ":"  ") + "Back");
        WriteFooterText("up/down, apply, back");
        return;
    }

    if (mode=="scenario_detail") {
        Title(CapFirst(scenario)+" details", 1, 1);
        double sMass = ScenarioMass(scenario, compMass, oreMass, iceMass);
        RenderDetail(CapFirst(scenario), sMass, shipMass, up, forward, upForward);
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Next slice");
        WriteLine((cursor==1?"> ":"  ") + "Back");
        WriteFooterText("up/down, apply, back");
        return;
    }

    if (mode=="scenario_slice") {
        Title(CapFirst(scenario)+" details", 1, 1);
        double sMass = ScenarioMass(scenario, compMass, oreMass, iceMass);
        double w = shipMass + sMass*(slice/100.0);
        WriteLine("Capacity at 1g - " + slice + "%");
        RenderAxisCapacity("UP  ", up, w);
        RenderAxisCapacity("DOWN", down, w);
        RenderAxisCapacity("LEFT", left, w);
        RenderAxisCapacity("RIGHT", right, w);
        RenderAxisCapacity("FWD ", forward, w);
        RenderAxisCapacity("BCK ", backward, w);
        RenderAxisCapacity("U+F ", upForward, w);
        WriteLine("");
        WriteLine((cursor==0?"> ":"  ") + "Next slice");
        WriteLine((cursor==1?"> ":"  ") + "Back");
        WriteFooterText("up/down, apply, back");
        return;
    }

    // Legacy page-based block removed; state machine handles all views.
}

// === Helpers ===
IMyShipController GetMainController() {
    for (int i = 0; i < controllers.Count; i++)
        if (controllers[i].IsMainCockpit) return controllers[i];
    return controllers[0];
}

void WriteLine(string text, bool append=true) {
    if (surface != null) surface.WriteText(text + "\n", append);
    else if (lcd != null) lcd.WriteText(text + "\n", append);
    else Echo(text);
}

// Footer nav hint
void WriteFooter(int page, int totalPages) {
    WriteLine("");
    if (page <= 1) WriteLine("next >");
    else if (page >= totalPages) WriteLine("< last");
    else WriteLine("< last        next >");
}

void WriteFooterText(string text) {
    WriteLine("");
    WriteLine(text);
}

// Title centered-ish
void Title(string name, int page, int total) {
    string t = "=== " + name + " - " + page + " / " + total + " ===";
    WriteLine(Center(t));
    WriteLine("");
}

string Center(string s) {
    int width = 40; // monospace approximation
    int pad = Math.Max(0, (width - s.Length) / 2);
    return new string(' ', pad) + s;
}

int MaxCursorMode(string m){
    if (m=="overview") return 2;                // thrust, scenarios, ship overview
    if (m=="thrust_overview") return 1;         // details, back
    if (m=="thrust_detail") return 0;           // back
    if (m=="scenario_overview") return 2;       // next, details, back
    if (m=="scenario_detail") return 1;         // next slice, back
    if (m=="scenario_slice") return 1;          // next slice, back
    if (m=="ship_overview") return 1;           // next slice, back
    return 0;
}

// Axis rendering helpers
void RenderAxisCapacity(string label, double thrustKg, double massKg) {
    double pct = (massKg<=0||thrustKg<=0)?0.0:(thrustKg/massKg)*100.0;
    WriteLine(label + " | " + Bar(pct,6) + " " + pct.ToString("0") + "%");
}
void RenderAxisRatio(string label, double massKg, double thrustKg) {
    double pct = (thrustKg<=0)?0.0:(massKg/thrustKg)*100.0;
    WriteLine(label + " | " + Bar(pct,6) + " " + pct.ToString("0") + "%");
}

double ScenarioMass(string scen, double comp, double ore, double ice) {
    if (scen=="comp") return comp; if (scen=="ore") return ore; if (scen=="ice") return ice; return 0.0;
}
string CapFirst(string s){ if(string.IsNullOrEmpty(s)) return s; return char.ToUpper(s[0])+s.Substring(1); }

int ComputeMaxLines(IMyTextSurface surf){
    var size = surf.SurfaceSize; // pixels
    // empirical line height factor for monospace
    float linePx = 22f * surf.FontSize; // tweak if needed
    int lines = (int)(size.Y / Math.Max(12f, linePx));
    return Math.Max(12, lines);
}

string ThrusterTypeTag(IMyThrust t){
    string n = t.DefinitionDisplayNameText ?? t.CustomName;
    string l = (n??"").ToLower();
    if (l.Contains("hydrogen")) return "Hydro";
    if (l.Contains("atmo")) return "Atmo";
    if (l.Contains("atmos")) return "Atmo";
    return "Ion";
}

string GridTag(IMyTerminalBlock b){
    return b.CubeGrid.GridSizeEnum == MyCubeSize.Large ? "LG" : "SG";
}

string ThrusterSizeTag(IMyThrust t){
    // Robust size detection across vanilla + DLC naming
    string d = (t.DefinitionDisplayNameText ?? "").ToLower();
    string s = (t.BlockDefinition.SubtypeName ?? "").ToLower();
    string ty = ThrusterTypeTag(t);

    // Named patterns in display name take precedence
    if (d.Contains(" large ") || d.StartsWith("large ")) return "Big";
    if (d.Contains(" small ") || d.StartsWith("small ")) return "Sml";

    // Industrial smalls typically omit "Large" in display name
    if (d.Contains("industrial") && !d.Contains(" large ")) return "Sml";

    // Subtype hints (vanilla naming)
    if (s.Contains("largeblocklarge")) return "Big";
    if (s.Contains("smallblocksmall")) return "Sml";
    if (s.Contains("largeblock")) return "Big";
    if (s.Contains("smallblock")) return "Sml";

    // Fallback by thrust magnitude per type (conservative thresholds for large grid)
    double n = t.MaxEffectiveThrust; // Newtons
    if (ty == "Hydro") return n > 1.2e7 ? "Big" : "Sml";   // ~12 MN threshold
    if (ty == "Ion")  return n > 3.0e6 ? "Big" : "Sml";    // ~3 MN threshold
    if (ty == "Atmo") return n > 2.0e6 ? "Big" : "Sml";    // ~2 MN threshold
    return n > 3.0e6 ? "Big" : "Sml";
}

string ContainerTypeTag(IMyCargoContainer c){
    string n = c.DefinitionDisplayNameText ?? c.CustomName; string l=(n??"").ToLower();
    if (l.Contains("small")) return "Small Cargo";
    if (l.Contains("medium")) return "Medium Cargo";
    if (l.Contains("large")) return "Large Cargo";
    return n;
}

string GroupGas(List<IMyGasTank> tanks, bool hydrogen, Dictionary<string,int> outMap){
    for (int i=0;i<tanks.Count;i++){
        var t=tanks[i]; string n=t.DefinitionDisplayNameText ?? t.CustomName; string l=(n??"").ToLower();
        bool isH = l.Contains("hydrogen"); bool isO = l.Contains("oxygen");
        if (hydrogen && !isH) continue; if (!hydrogen && !isO) continue;
        string key = n;
        if (!outMap.ContainsKey(key)) outMap[key]=0; outMap[key]++;
    }
    if (outMap.Count==0) return "- none -";
    var sb = new System.Text.StringBuilder();
    foreach (var kv in outMap) { sb.Append("  ").Append(kv.Key).Append(": x").Append(kv.Value).Append("\n"); }
    return sb.ToString().TrimEnd();
}

// Build ship overview sections; each section is a list of lines, kept intact per slice
List<List<string>> BuildShipSections(MatrixD refMatrix){
    var sections = new List<List<string>>();

    // Thrusters by side (compact table, no grid in label)
    var thr = new List<string>();
    thr.Add("Thrusters (IS IB HS HB AS AB)");
    thr.Add("Side | IS IB HS HB AS AB");
    string[] sides = new[]{"Up","Down","Left","Right","Fwd","Back"};
    var sideMap = new Dictionary<string, Dictionary<string,int>>();
    for (int i=0;i<sides.Length;i++) sideMap[sides[i]] = new Dictionary<string,int>();
    for (int i=0;i<thrusters.Count;i++){
        var t = thrusters[i];
        Vector3D local = Vector3D.TransformNormal(-t.WorldMatrix.Forward, MatrixD.Transpose(refMatrix));
        string side=null;
        if (local.Y > 0.9) side="Up"; else if (local.Y < -0.9) side="Down";
        else if (local.Z < -0.9) side="Fwd"; else if (local.Z > 0.9) side="Back";
        else if (local.X > 0.9) side="Right"; else if (local.X < -0.9) side="Left";
        if (side==null) continue;
        string ty = ThrusterTypeTag(t) + " " + ThrusterSizeTag(t); // no grid in label
        var map = sideMap[side]; if (!map.ContainsKey(ty)) map[ty]=0; map[ty]++;
    }
    for (int i=0;i<sides.Length;i++){
        var c = sideMap[sides[i]];
        int isml = CountOrZero(c, "Ion Sml"), ibig = CountOrZero(c, "Ion Big");
        int hsml = CountOrZero(c, "Hydro Sml"), hbig = CountOrZero(c, "Hydro Big");
        int asml = CountOrZero(c, "Atmo Sml"), abig = CountOrZero(c, "Atmo Big");
        string line = sides[i].PadRight(4) + "| "
            + isml.ToString().PadLeft(2) + " " + ibig.ToString().PadLeft(2) + " "
            + hsml.ToString().PadLeft(2) + " " + hbig.ToString().PadLeft(2) + " "
            + asml.ToString().PadLeft(2) + " " + abig.ToString().PadLeft(2);
        thr.Add(line);
    }
    sections.Add(thr);

    // Cargo containers
    var cargo = new List<string>();
    cargo.Add("Cargo containers:");
    var cMap = new Dictionary<string,int>();
    for (int i=0;i<cargoContainers.Count;i++){ var c=cargoContainers[i]; string key=ContainerTypeTag(c); if(!cMap.ContainsKey(key)) cMap[key]=0; cMap[key]++; }
    if (cMap.Count==0) cargo.Add("- none -"); else { foreach (var kv in cMap) cargo.Add("  "+kv.Key+": x"+kv.Value); }
    sections.Add(cargo);

    // H2 tanks
    var hsec = new List<string>(); hsec.Add("H2 tanks:");
    var hMap = new Dictionary<string,int>(); string hLines = GroupGas(gasTanks, true, hMap); foreach (var ln in hLines.Split('\n')) hsec.Add(ln);
    sections.Add(hsec);
    // O2 tanks
    var osec = new List<string>(); osec.Add("O2 tanks:");
    var oMap = new Dictionary<string,int>(); string oLines = GroupGas(gasTanks, false, oMap); foreach (var ln in oLines.Split('\n')) osec.Add(ln);
    sections.Add(osec);

    return sections;
}

// Safe lookup for compact table counts
int CountOrZero(Dictionary<string,int> map, string key){
    int v; return map != null && map.TryGetValue(key, out v) ? v : 0;
}

// Sum total hydrogen capacity (L) on this grid
double TotalHydrogenCapacityL(){
    double cap=0; for (int i=0;i<gasTanks.Count;i++){
        var t = gasTanks[i]; string n = (t.DefinitionDisplayNameText ?? t.CustomName ?? "").ToLower();
        if (n.Contains("hydrogen")) cap += t.Capacity; // liters
    }
    return cap;
}

// Estimate hydro percent to climb distance at 100 m/s with small buffer
double EstimateHydroPercent(double massKg, MatrixD refMatrix, double distanceMeters){
    double hydroCapL = TotalHydrogenCapacityL(); if (hydroCapL <= 0) return -1;
    // Required hover force (N) at 1g
    double neededN = massKg * 9.81;
    // Hydro up thrust available (N)
    double hydroUpN = 0;
    for (int i=0;i<thrusters.Count;i++){
        var t = thrusters[i];
        string name = (t.DefinitionDisplayNameText ?? t.CustomName ?? "").ToLower();
        if (!name.Contains("hydrogen")) continue;
        Vector3D local = Vector3D.TransformNormal(-t.WorldMatrix.Forward, MatrixD.Transpose(refMatrix));
        if (local.Y > 0.9) hydroUpN += t.MaxEffectiveThrust; // N
    }
    if (hydroUpN <= 0) return -1;
    double time = (distanceMeters / 100.0) * 1.2; // 100 m/s + 20% buffer
    // Assume hydrogen flow ~ k * thrust(N). Calibrated k so a 12 MN hydro burns ~1440 L/s
    const double kL_per_Ns = 1.2e-4; // liters per N*second (rough)
    // At steady 100 m/s, assume thrust ~= weight (no acceleration). Cap at available hydro.
    double usedN = Math.Min(neededN, hydroUpN);
    double liters = kL_per_Ns * usedN * time;
    return Math.Min(999.0, liters / hydroCapL * 100.0);
}

// Tiny progress bar + percentage cell for one column
string Cell(double scenarioMass, double factor, double baseMass, double axisThrust) {
    double req = baseMass + scenarioMass * factor;
    if (req <= 0 || axisThrust <= 0) return "[......] 0%";
    double pct = (axisThrust / req) * 100.0;
    return Bar(pct, 6) + " " + pct.ToString("0") + "%";
}

// Render a single row for Up/Fw/U+F across 25/50 and 75/100 (two lines)
void RenderScenarioRow(string label, double scenarioMass, double baseMass, double axisThrust) {
    string c25 = Cell(scenarioMass, 0.25, baseMass, axisThrust);
    string c50 = Cell(scenarioMass, 0.50, baseMass, axisThrust);
    string c75 = Cell(scenarioMass, 0.75, baseMass, axisThrust);
    string c100 = Cell(scenarioMass, 1.00, baseMass, axisThrust);
    WriteLine(label + " | " + c25 + " | " + c50);
    WriteLine("   | " + c75 + " | " + c100);
}

// Detail view for a scenario with 25% slices
void RenderDetail(string title, double scenarioMass, double baseMass, double upT, double fwT, double ufT) {
    double w25 = baseMass + scenarioMass*0.25;
    double w50 = baseMass + scenarioMass*0.50;
    double w75 = baseMass + scenarioMass*0.75;
    double w100 = baseMass + scenarioMass;
    WriteLine("Scenario detail:");
    WriteLine("[" + title + "]");
    WriteLine("Ship mass: " + Fm(baseMass));
    WriteLine("Cargo mass:");
    WriteLine("  25%: " + Fm(scenarioMass*0.25) + " | 50%: " + Fm(scenarioMass*0.50));
    WriteLine("  75%: " + Fm(scenarioMass*0.75) + " | 100%: " + Fm(scenarioMass));
    WriteLine("");
    WriteLine("Ship + cargo:");
    WriteLine("  25%: " + Fm(w25) + " | 50%: " + Fm(w50));
    WriteLine("  75%: " + Fm(w75) + " | 100%: " + Fm(w100));
}

// Percent capacity helper (inverse of Ratio)
string Capacity(double mass, double thrustKg) {
    if (thrustKg <= 0) return "N/A";
    double cap = (thrustKg / mass) * 100.0;
    return cap.ToString("0.0") + "%";
}

// Draw a tiny ASCII progress bar; clamp overfill
string Bar(double pct, int width) {
    if (pct < 0) pct = 0; if (pct > 999) pct = 999; // avoid crazy numbers blowing width
    int fill = (int)Math.Round(Math.Min(100.0, pct) / 100.0 * width);
    return "[" + new string('#', fill) + new string('.', Math.Max(0, width - fill)) + "]";
}

string Ratio(double mass,double thrustKg){
    if(thrustKg<=0)return"N/A";
    double ratio=(mass/thrustKg)*100.0;
    return ratio.ToString("0.0")+"%";
}
string Est(double full,double r,double baseM,double thrust){
    double add=full*r; double tot=baseM+add; double pct=(tot/thrust)*100;
    string w=Fm(add); return w+"("+pct.ToString("0")+"%)";
}
string Fm(double kg){if(kg>1e6)return(kg/1e6).ToString("0.00")+"Mt";
if(kg>1e3)return(kg/1e3).ToString("0.0")+"t";return kg.ToString("0")+"kg";}
string Fv(double L){if(L>1e6)return(L/1e6).ToString("0.00")+"ML";
if(L>1e3)return(L/1e3).ToString("0.0")+"kL";return L.ToString("0")+"L";}
string Pad(string s,int len){while(s.Length<len)s+=" ";return s;}
