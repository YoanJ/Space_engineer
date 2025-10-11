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
        else if (local.Z > 0.9) forward += thrustKg;
        else if (local.Z < -0.9) backward += thrustKg;
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

    // === Pagination + persisted state ===
    int TOTAL_PAGES = 6; // 1:Overview, 2:Thrust-1g, 3:Empty, 4:Comp, 5:Ore, 6:Ice
    int page = 1;
    string savedDetail = null;
    if (!string.IsNullOrEmpty(Storage)) {
        var tokens = Storage.Split(';');
        for (int i=0;i<tokens.Length;i++) {
            var t = tokens[i];
            if (t.StartsWith("p=")) int.TryParse(t.Substring(2), out page);
            else if (t.StartsWith("d=")) savedDetail = t.Substring(2);
        }
    }
    string arg = (argument ?? "").Trim().ToLower();
    if (arg == "next") page = page >= TOTAL_PAGES ? 1 : page + 1;
    else if (arg == "last") page = page <= 1 ? TOTAL_PAGES : page - 1;
    if (page < 1 || page > TOTAL_PAGES) page = 1;

    // Parse detail commands
    string detail = savedDetail;
    if (arg == "comp" || arg == "ore" || arg == "ice" || arg == "empty") detail = arg;
    if (arg == "back") detail = null;

    // Clear redundant headers; keep things compact

    if (page == 1) {
        Title("Overview", page, TOTAL_PAGES);
        WriteLine("Mass: " + Fm(shipMass));
        Vector3D gVec = ctrl.GetNaturalGravity();
        double g = gVec.Length();
        WriteLine("Gravity: " + g.ToString("0.00") + " m/s^2 (" + (g/9.81).ToString("0.00") + " g)");
        WriteLine("Cargo: " + Fv(usedL) + "/" + Fv(totalL) + " (" + fill.ToString("0.0") + "%)");
        WriteLine("");
        // Persist state
        Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
        WriteFooter(page, TOTAL_PAGES);
        return;
    }

    // Page 2: Thrust by side - 1g
    if (page == 2) {
        Title("Thrust by Side - 1g", page, TOTAL_PAGES);
        WriteLine("kgf totals");
        WriteLine("Up:" + Pad(Fm(up),8) + "  Down:" + Fm(down));
        WriteLine("Fwd:" + Pad(Fm(forward),8) + "  Back:" + Fm(backward));
        WriteLine("Left:" + Pad(Fm(left),8) + "  Right:" + Fm(right));
        WriteLine("U+F: " + Fm(upForward));
        WriteLine("");
        WriteLine("Load Ratio (Empty) @1g");
        WriteLine("Up: " + Ratio(shipMass, up) + "   Down: " + Ratio(shipMass, down));
        WriteLine("Left: " + Ratio(shipMass, left) + "   Right: " + Ratio(shipMass, right));
        WriteLine("Fwd: " + Ratio(shipMass, forward) + "   Back: " + Ratio(shipMass, backward));
        WriteLine("U+F: " + Ratio(shipMass, upForward));
        Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
        WriteFooter(page, TOTAL_PAGES);
        return;
    }

    // Page 3: Scenario Empty (no detail, no fill table)
    if (page == 3) {
        Title("Scenario", page, TOTAL_PAGES);
        WriteLine("[Empty]");
        WriteLine("");
        // Show progress bars per slice (identical values since scenario mass = 0)
        WriteLine("Fill / Thrusters   25% | 50%");
        WriteLine("                   75% | 100%");
        RenderScenarioRow("Up ", 0.0, shipMass, up);
        RenderScenarioRow("Fw ", 0.0, shipMass, forward);
        RenderScenarioRow("U+F", 0.0, shipMass, upForward);
        Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
        WriteFooter(page, TOTAL_PAGES);
        return;
    }

    // Page 4: Scenario Components
    if (page == 4) {
        if (detail == "comp" || detail == "empty") {
            Title("Scenario", page, TOTAL_PAGES);
            RenderDetail(detail == "comp" ? "Components" : "Empty", detail == "comp" ? compMass : 0.0, shipMass, up, forward, upForward);
            WriteLine("");
            WriteLine("< back");
            Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
            return;
        }
        Title("Scenario", page, TOTAL_PAGES);
        WriteLine("[Components]");
        if (arg == "ok") detail = "comp";
        WriteLine("Fill / Thrusters   25% | 50%");
        WriteLine("                   75% | 100%");
        RenderScenarioRow("Up ", compMass, shipMass, up);
        RenderScenarioRow("Fw ", compMass, shipMass, forward);
        RenderScenarioRow("U+F", compMass, shipMass, upForward);
        Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
        WriteFooter(page, TOTAL_PAGES);
        return;
    }

    // Page 5: Scenario Ore
    if (page == 5) {
        Title("Scenario", page, TOTAL_PAGES);
        WriteLine("[Ore]");
        if (arg == "ok") detail = "ore";
        if (detail == "ore") {
            bool isOre = detail == "ore";
            RenderDetail(isOre ? "Ore" : "Ice", isOre ? oreMass : iceMass, shipMass, up, forward, upForward);
            WriteLine("");
            WriteLine("< back");
            Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
            return;
        }
        WriteLine("Fill / Thrusters   25% | 50%");
        WriteLine("                   75% | 100%");
        RenderScenarioRow("Up ", oreMass, shipMass, up);
        RenderScenarioRow("Fw ", oreMass, shipMass, forward);
        RenderScenarioRow("U+F", oreMass, shipMass, upForward);
        Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
        WriteFooter(page, TOTAL_PAGES);
        return;
    }

    // Page 6: Scenario Ice
    if (page == 6) {
        Title("Scenario", page, TOTAL_PAGES);
        WriteLine("[Ice]");
        if (arg == "ok") detail = "ice";
        if (detail == "ice") {
            RenderDetail("Ice", iceMass, shipMass, up, forward, upForward);
            WriteLine("");
            WriteLine("< back");
            Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
            return;
        }
        WriteLine("Fill / Thrusters   25% | 50%");
        WriteLine("                   75% | 100%");
        RenderScenarioRow("Up ", iceMass, shipMass, up);
        RenderScenarioRow("Fw ", iceMass, shipMass, forward);
        RenderScenarioRow("U+F", iceMass, shipMass, upForward);
        Storage = "p=" + page.ToString() + ";d=" + (detail ?? "");
        WriteFooter(page, TOTAL_PAGES);
        return;
    }
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

// Tiny progress bar + percentage cell for one column
string Cell(double scenarioMass, double factor, double baseMass, double axisThrust) {
    double add = scenarioMass * factor;
    double pct = ((baseMass + add) / axisThrust) * 100.0; // >100% = cannot lift
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
    WriteLine("Estimated weight:");
    WriteLine("  25%:  " + Fm(w25));
    WriteLine("  50%:  " + Fm(w50));
    WriteLine("  75%:  " + Fm(w75));
    WriteLine("  100%: " + Fm(w100));
    WriteLine("Capacity (1g):");
    WriteLine("        25%    50%");
    WriteLine("  Up:  " + Capacity(w25, upT) + " | " + Capacity(w50, upT));
    WriteLine("        75%    100%");
    WriteLine("       " + Capacity(w75, upT) + " | " + Capacity(w100, upT));
    WriteLine("");
    WriteLine("        25%    50%");
    WriteLine("  Fw:  " + Capacity(w25, fwT) + " | " + Capacity(w50, fwT));
    WriteLine("        75%    100%");
    WriteLine("       " + Capacity(w75, fwT) + " | " + Capacity(w100, fwT));
    WriteLine("");
    WriteLine("        25%    50%");
    WriteLine("  U+F: " + Capacity(w25, ufT) + " | " + Capacity(w50, ufT));
    WriteLine("        75%    100%");
    WriteLine("       " + Capacity(w75, ufT) + " | " + Capacity(w100, ufT));
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
