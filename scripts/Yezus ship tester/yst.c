// === [YST Ship Load Overview v3.8 - Titles, pages, detail nav] ===
// Works on cockpit screens tagged [YST:index] or panels [YST]
// Accurate thrust & load ratios, multi-page LCD, 0g compatible

List<IMyThrust> thrusters = new List<IMyThrust>();
List<IMyShipController> controllers = new List<IMyShipController>();
List<IMyTerminalBlock> containers = new List<IMyTerminalBlock>();
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
    string mode = "overview", scenario = "empty"; int slice = 25; int cursor = 0;
    if (!string.IsNullOrEmpty(Storage)) {
        var parts = Storage.Split(';');
        for (int i=0;i<parts.Length;i++) {
            string p = parts[i];
            if (p.StartsWith("mode=")) mode = p.Substring(5);
            else if (p.StartsWith("scenario=")) scenario = p.Substring(9);
            else if (p.StartsWith("slice=")) int.TryParse(p.Substring(6), out slice);
            else if (p.StartsWith("cursor=")) int.TryParse(p.Substring(7), out cursor);
        }
    }

    string arg = (argument ?? "").Trim().ToLower();
    if (arg == "comp" || arg == "ore" || arg == "ice") { mode = "scenario_overview"; scenario = arg; cursor=0; }
    if (arg == "empty") { mode = "thrust_overview"; cursor=0; }
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
    }
    else if (arg == "apply") {
        if (mode=="overview") {
            if (cursor==0) { mode = "thrust_overview"; cursor=0; }
            else if (cursor==1) { mode = "scenario_overview"; if (string.IsNullOrEmpty(scenario)) scenario="comp"; cursor=0; }
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
        }
    }

    // Save
    Storage = "mode="+mode+";scenario="+scenario+";slice="+slice.ToString()+";cursor="+cursor.ToString();

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
        WriteFooterText("up/down, apply, menu");
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
    if (m=="overview") return 1;                // thrust, scenarios
    if (m=="thrust_overview") return 1;         // details, back
    if (m=="thrust_detail") return 0;           // back
    if (m=="scenario_overview") return 2;       // next, details, back
    if (m=="scenario_detail") return 1;         // next slice, back
    if (m=="scenario_slice") return 1;          // next slice, back
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
