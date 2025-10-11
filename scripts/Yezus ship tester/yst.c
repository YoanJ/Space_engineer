// === [YST Ship Load Overview v3.6 - Cockpit Display Support, C#6 Compatible] ===
// Works on cockpit screens tagged [YST:index] or panels [YST] Main
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

    // 1️⃣ Priorité : LCD panel avec [YST]
    List<IMyTextPanel> panels = new List<IMyTextPanel>();
    GridTerminalSystem.GetBlocksOfType(panels, p => p.CubeGrid == Me.CubeGrid && p.CustomName.Contains("[YST]"));
    if (panels.Count > 0) {
        lcd = panels[0];
        surface = lcd as IMyTextSurface;
        return;
    }

    // 2️⃣ Sinon, cockpit taggé [YST:index]
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

    // === Pagination ===
    int page = 1;
    if (!string.IsNullOrEmpty(Storage)) int.TryParse(Storage, out page);
    string arg = (argument ?? "").Trim().ToLower();
    if (arg == "next") page = page >= 3 ? 1 : page + 1;
    else if (arg == "last") page = page <= 1 ? 3 : page - 1;
    if (page < 1 || page > 3) page = 1;
    Storage = page.ToString();

    WriteLine("Page " + page + "/3  (args: next / last)");
    WriteLine("");

    if (page == 1) {
        WriteLine("=== PAGE 1: Overview ===");
        WriteLine("Mass: " + Fm(shipMass));
        Vector3D gVec = ctrl.GetNaturalGravity();
        double g = gVec.Length();
        WriteLine("Gravity: " + g.ToString("0.00") + " m/s² (" + (g/9.81).ToString("0.00") + " g)");
        WriteLine("Cargo: " + Fv(usedL) + "/" + Fv(totalL) + " (" + fill.ToString("0.0") + "%)");
        WriteLine("");
        WriteLine("=== Thrust by Side (kgf) ===");
        WriteLine("Up:" + Pad(Fm(up),8) + "  Down:" + Fm(down));
        WriteLine("Fwd:" + Pad(Fm(forward),8) + "  Back:" + Fm(backward));
        WriteLine("Left:" + Pad(Fm(left),8) + "  Right:" + Fm(right));
        WriteLine("Up+Fw(45°): " + Fm(upForward));
        return;
    }

    if (page == 2) {
        WriteLine("=== PAGE 2: Components ===");
        WriteLine("Empty Load Ratio -> Up: " + Ratio(shipMass, up) + "  Fw: " + Ratio(shipMass, forward) + "  U+F: " + Ratio(shipMass, upForward));
        WriteLine("");
        WriteLine("Axis | 25% | 50% | 75% |100%|");
        WriteLine("Up  | " + Est(compMass,0.25,shipMass,up) + " | " + Est(compMass,0.5,shipMass,up) + " | " + Est(compMass,0.75,shipMass,up) + " | " + Est(compMass,1.0,shipMass,up) + " |");
        WriteLine("Fw  | " + Est(compMass,0.25,shipMass,forward) + " | " + Est(compMass,0.5,shipMass,forward) + " | " + Est(compMass,0.75,shipMass,forward) + " | " + Est(compMass,1.0,shipMass,forward) + " |");
        WriteLine("U+F| " + Est(compMass,0.25,shipMass,upForward) + " | " + Est(compMass,0.5,shipMass,upForward) + " | " + Est(compMass,0.75,shipMass,upForward) + " | " + Est(compMass,1.0,shipMass,upForward) + " |");
        return;
    }

    if (page == 3) {
        WriteLine("=== PAGE 3: Ore & Ice ===");
        WriteLine("Axis | 25% | 50% | 75% |100%|");
        WriteLine("Ore-Up| " + Est(oreMass,0.25,shipMass,up) + " | " + Est(oreMass,0.5,shipMass,up) + " | " + Est(oreMass,0.75,shipMass,up) + " | " + Est(oreMass,1.0,shipMass,up) + " |");
        WriteLine("Ore-Fw| " + Est(oreMass,0.25,shipMass,forward) + " | " + Est(oreMass,0.5,shipMass,forward) + " | " + Est(oreMass,0.75,shipMass,forward) + " | " + Est(oreMass,1.0,shipMass,forward) + " |");
        WriteLine("Ore-UF| " + Est(oreMass,0.25,shipMass,upForward) + " | " + Est(oreMass,0.5,shipMass,upForward) + " | " + Est(oreMass,0.75,shipMass,upForward) + " | " + Est(oreMass,1.0,shipMass,upForward) + " |");
        WriteLine("");
        WriteLine("Ice-Up| " + Est(iceMass,0.25,shipMass,up) + " | " + Est(iceMass,0.5,shipMass,up) + " | " + Est(iceMass,0.75,shipMass,up) + " | " + Est(iceMass,1.0,shipMass,up) + " |");
        WriteLine("Ice-Fw| " + Est(iceMass,0.25,shipMass,forward) + " | " + Est(iceMass,0.5,shipMass,forward) + " | " + Est(iceMass,0.75,shipMass,forward) + " | " + Est(iceMass,1.0,shipMass,forward) + " |");
        WriteLine("Ice-UF| " + Est(iceMass,0.25,shipMass,upForward) + " | " + Est(iceMass,0.5,shipMass,upForward) + " | " + Est(iceMass,0.75,shipMass,upForward) + " | " + Est(iceMass,1.0,shipMass,upForward) + " |");
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
