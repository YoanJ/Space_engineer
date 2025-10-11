// === [YST Ship Load Overview v3.5 - Empty + Up+Fw combo, C#6 Compatible] ===
// Accurate thrust & load ratios relative to cockpit, works even in 0g

List<IMyThrust> thrusters = new List<IMyThrust>();
List<IMyShipController> controllers = new List<IMyShipController>();
List<IMyTerminalBlock> containers = new List<IMyTerminalBlock>();
IMyTextPanel lcd;

public Program() {
    Runtime.UpdateFrequency = UpdateFrequency.None;
    RefreshBlocks();
}

void RefreshBlocks() {
    lcd = null;
    GridTerminalSystem.GetBlocksOfType(thrusters, t => t.CubeGrid == Me.CubeGrid);
    GridTerminalSystem.GetBlocksOfType(controllers, c => c.CubeGrid == Me.CubeGrid);
    GridTerminalSystem.GetBlocksOfType(containers, c =>
        c.CubeGrid == Me.CubeGrid && (c.HasInventory && !(c is IMyGasTank)));

    List<IMyTextPanel> panels = new List<IMyTextPanel>();
    GridTerminalSystem.GetBlocksOfType(panels, p => p.CustomName.Contains("[YST] Main"));
    if (panels.Count > 0) lcd = panels[0];
}

public void Main(string argument, UpdateType updateSource) {
    if (lcd == null) {
        RefreshBlocks();
        if (lcd == null) {
            Echo("No LCD [YST] Main found.");
            return;
        }
    }

    lcd.ContentType = ContentType.TEXT_AND_IMAGE;
    lcd.Font = "Monospace";
    lcd.FontSize = 0.7f;
    lcd.Alignment = TextAlignment.LEFT;
    lcd.WriteText("", false);

    if (controllers.Count == 0) {
        lcd.WriteText("No cockpit/RC found.", false);
        return;
    }

    IMyShipController ctrl = GetMainController();
    MatrixD refMatrix = ctrl.WorldMatrix;
    double shipMass = ctrl.CalculateShipMass().PhysicalMass;

    // === Directional thrust (relative to cockpit) ===
    double up = 0, down = 0, left = 0, right = 0, forward = 0, backward = 0;
    foreach (var t in thrusters) {
        double thrustKg = t.MaxEffectiveThrust * 0.1019716213;
        Vector3D thrDir = t.WorldMatrix.Forward;
        Vector3D local = Vector3D.TransformNormal(thrDir, MatrixD.Transpose(refMatrix));

        if (local.Y > 0.9) up += thrustKg;
        else if (local.Y < -0.9) down += thrustKg;
        else if (local.Z > 0.9) forward += thrustKg;
        else if (local.Z < -0.9) backward += thrustKg;
        else if (local.X > 0.9) right += thrustKg;
        else if (local.X < -0.9) left += thrustKg;
    }

    // === Cargo ===
    double totalVol = 0, usedVol = 0;
    for (int i = 0; i < containers.Count; i++)
        for (int inv = 0; inv < containers[i].InventoryCount; inv++) {
            var invRef = containers[i].GetInventory(inv);
            totalVol += (double)invRef.MaxVolume;
            usedVol += (double)invRef.CurrentVolume;
        }

    double totalL = totalVol * 1000, usedL = usedVol * 1000;
    double fill = (totalL > 0) ? (usedL / totalL * 100) : 0;

    // === Densités ===
    double compD = 0.7, oreD = 2.5, iceD = 0.9;
    double compMass = totalL * compD, oreMass = totalL * oreD, iceMass = totalL * iceD;

    // === Combinaison Up + Forward (45°) ===
    double upForward = (up * 0.707) + (forward * 0.707);

    // === Output ===
    lcd.WriteText("=== Ship Overview ===\n", true);
    lcd.WriteText("Mass: " + Fm(shipMass) + "\n", true);
    lcd.WriteText("Cargo: " + Fv(usedL) + "/" + Fv(totalL) + " (" + fill.ToString("0.0") + "%)\n\n", true);

    lcd.WriteText("=== Thrust by Side (kgf) ===\n", true);
    lcd.WriteText("Up:" + Pad(Fm(up),8) + "  Down:" + Fm(down) + "\n", true);
    lcd.WriteText("Fwd:" + Pad(Fm(forward),8) + "  Back:" + Fm(backward) + "\n", true);
    lcd.WriteText("Left:" + Pad(Fm(left),8) + "  Right:" + Fm(right) + "\n", true);
    lcd.WriteText("Up+Fw (45°): " + Fm(upForward) + "\n\n", true);

    // === Load ratio empty ===
    lcd.WriteText("=== Load Ratio (Empty) ===\n", true);
    lcd.WriteText("Up: " + Ratio(shipMass, up) + "   Fw: " + Ratio(shipMass, forward) + "   Up+Fw: " + Ratio(shipMass, upForward) + "\n\n", true);

    // === Estimated ratios ===
    lcd.WriteText("=== Est. Load Ratios ===\n", true);
    lcd.WriteText("Scenario | Axis | 25% | 50% | 75% |100%|\n", true);

    lcd.WriteText("Comp | Up | " + Est(compMass,0.25,shipMass,up) + " | " + Est(compMass,0.5,shipMass,up) + " | " + Est(compMass,0.75,shipMass,up) + " | " + Est(compMass,1.0,shipMass,up) + " |\n", true);
    lcd.WriteText("      | Fw | " + Est(compMass,0.25,shipMass,forward) + " | " + Est(compMass,0.5,shipMass,forward) + " | " + Est(compMass,0.75,shipMass,forward) + " | " + Est(compMass,1.0,shipMass,forward) + " |\n", true);
    lcd.WriteText("      | U+F | " + Est(compMass,0.25,shipMass,upForward) + " | " + Est(compMass,0.5,shipMass,upForward) + " | " + Est(compMass,0.75,shipMass,upForward) + " | " + Est(compMass,1.0,shipMass,upForward) + " |\n", true);

    lcd.WriteText("Ore  | Up | " + Est(oreMass,0.25,shipMass,up) + " | " + Est(oreMass,0.5,shipMass,up) + " | " + Est(oreMass,0.75,shipMass,up) + " | " + Est(oreMass,1.0,shipMass,up) + " |\n", true);
    lcd.WriteText("      | Fw | " + Est(oreMass,0.25,shipMass,forward) + " | " + Est(oreMass,0.5,shipMass,forward) + " | " + Est(oreMass,0.75,shipMass,forward) + " | " + Est(oreMass,1.0,shipMass,forward) + " |\n", true);
    lcd.WriteText("      | U+F | " + Est(oreMass,0.25,shipMass,upForward) + " | " + Est(oreMass,0.5,shipMass,upForward) + " | " + Est(oreMass,0.75,shipMass,upForward) + " | " + Est(oreMass,1.0,shipMass,upForward) + " |\n", true);

    lcd.WriteText("Ice  | Up | " + Est(iceMass,0.25,shipMass,up) + " | " + Est(iceMass,0.5,shipMass,up) + " | " + Est(iceMass,0.75,shipMass,up) + " | " + Est(iceMass,1.0,shipMass,up) + " |\n", true);
    lcd.WriteText("      | Fw | " + Est(iceMass,0.25,shipMass,forward) + " | " + Est(iceMass,0.5,shipMass,forward) + " | " + Est(iceMass,0.75,shipMass,forward) + " | " + Est(iceMass,1.0,shipMass,forward) + " |\n", true);
    lcd.WriteText("      | U+F | " + Est(iceMass,0.25,shipMass,upForward) + " | " + Est(iceMass,0.5,shipMass,upForward) + " | " + Est(iceMass,0.75,shipMass,upForward) + " | " + Est(iceMass,1.0,shipMass,upForward) + " |\n", true);

    lcd.WriteText("\n> Ratio >100% = cannot lift that load under 1g", true);
}

// === Helpers ===
IMyShipController GetMainController() {
    for (int i = 0; i < controllers.Count; i++)
        if (controllers[i].IsMainCockpit) return controllers[i];
    return controllers[0];
}

string Ratio(double mass, double thrustKg) {
    if (thrustKg <= 0) return "N/A";
    double ratio = (mass / thrustKg) * 100.0;
    return ratio.ToString("0.0") + "%";
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
