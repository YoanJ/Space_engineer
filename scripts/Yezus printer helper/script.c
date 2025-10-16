// Yezus Printer Helper - Sensor Auto-Ranging for Welder Grids
// -----------------------------------------------------------
// This programmable block script tunes a sensor block so that its detection box
// tightly encloses every welder on a target grid (plus a configurable padding).
//
// Typical use case: place a sensor on the printer frame, tag the sensor and the
// welders (or group the welders), paste the configuration template below into the
// programmable block Custom Data, and run the script. When AutoUpdate = true the
// script re-evaluates every 100 ticks so piston/rotor driven printers stay in sync.

using System;
using System.Collections.Generic;
using System.Text;
using Sandbox.ModAPI.Ingame;
using SpaceEngineers.Game.ModAPI.Ingame;
using VRage.Game.GUI.TextPanel;
using VRage.Game.ModAPI.Ingame.Utilities;
using VRageMath;

const string IniSection = "SensorSetup";
readonly MyIni _ini = new MyIni();

IMySensorBlock _sensor;
readonly List<IMyShipWelder> _welders = new List<IMyShipWelder>();
readonly List<IMySensorBlock> _sensorBuffer = new List<IMySensorBlock>();
readonly StringBuilder _sb = new StringBuilder(512);
IMyTextSurface _statusSurface;

string _sensorName;
string _welderGroupName;
string _welderTag;
double _padding;
bool _autoUpdate;
bool _sameGridOnly;
bool _sameConstructOnly;
bool _debug;

bool _configReady;
string _configSignature = string.Empty;
bool _needBlockRefresh = true;

public Program()
{
    _statusSurface = Me.GetSurface(0);
    ConfigureSurface(_statusSurface);
    BeginStatusFrame("Init", UpdateType.None);
    EnsureIniTemplate();
    LoadConfig();
    RebuildCache();
    AlignSensor();
    UpdateFrequency();
    WriteStatusSurface();
}

public void Main(string argument, UpdateType updateSource)
{
    string rawArgument = (argument ?? string.Empty).Trim();
    string command = rawArgument.ToLowerInvariant();
    BeginStatusFrame(string.IsNullOrEmpty(rawArgument) ? "<default>" : rawArgument, updateSource);

    if (command == "help")
    {
        EchoHelp();
        WriteStatusSurface();
        return;
    }

    bool configChanged = LoadConfig();
    bool forceRefresh = command == "refresh" || command == "scan";
    if (configChanged || forceRefresh)
    {
        RebuildCache();
    }

    if (_autoUpdate && (updateSource & UpdateType.Update100) == 0 && !forceRefresh && !configChanged)
    {
        // In auto mode the main work happens on Update100. We still honour manual triggers.
        WriteStatusSurface();
        return;
    }

    AlignSensor();
    UpdateFrequency();
    WriteStatusSurface();
}

void UpdateFrequency()
{
    Runtime.UpdateFrequency = _autoUpdate ? UpdateFrequency.Update100 : UpdateFrequency.None;
}

bool LoadConfig()
{
    _configReady = false;

    MyIniParseResult result;
    if (!_ini.TryParse(Me.CustomData, out result))
    {
        Echo($"CustomData error at line {result.LineNumber}: {result.Error}");
        return false;
    }

    _sensorName = _ini.Get(IniSection, "SensorName").ToString();
    _welderGroupName = _ini.Get(IniSection, "WelderGroup").ToString();
    _welderTag = _ini.Get(IniSection, "WelderTag").ToString();
    _padding = Math.Max(0.0, _ini.Get(IniSection, "PaddingMeters").ToDouble(0.5));
    _autoUpdate = _ini.Get(IniSection, "AutoUpdate").ToBoolean(false);
    _sameGridOnly = _ini.Get(IniSection, "OnlySensorGrid").ToBoolean(true);
    _sameConstructOnly = _ini.Get(IniSection, "SameConstruct").ToBoolean(true);
    _debug = _ini.Get(IniSection, "Debug").ToBoolean(false);

    _configReady = true;
    return true;
}

void EnsureIniTemplate()
{
    if (!string.IsNullOrWhiteSpace(Me.CustomData))
        return;

    _ini.Clear();
    _ini.Set(IniSection, "SensorName", "[Printer Sensor]");
    _ini.Set(IniSection, "WelderGroup", "");
    _ini.Set(IniSection, "WelderTag", "[Welder]");
    _ini.Set(IniSection, "PaddingMeters", 0.5);
    _ini.Set(IniSection, "AutoUpdate", false);
    _ini.Set(IniSection, "OnlySensorGrid", true);
    _ini.Set(IniSection, "SameConstruct", true);
    _ini.Set(IniSection, "Debug", false);
    _ini.Set(IniSection, "Notes", "Commands: run=default, refresh=reload blocks, help=show manual");
    Me.CustomData = _ini.ToString();
}

void RebuildCache()
{
    if (!_configReady)
        return;

    _sensor = null;
    _sensorBuffer.Clear();

    GridTerminalSystem.GetBlocksOfType(_sensorBuffer, s => BlockMatchesSensorFilter(s));
    if (_sensorBuffer.Count > 0)
    {
        _sensor = _sensorBuffer[0];
    }

    if (_sensor == null)
    {
        Echo("Sensor not found. Check SensorName filter.");
        return;
    }

    _welders.Clear();
    ulong sensorGridId = _sensor.CubeGrid.EntityId;

    if (!string.IsNullOrWhiteSpace(_welderGroupName))
    {
        var group = GridTerminalSystem.GetBlockGroupWithName(_welderGroupName);
        if (group == null)
        {
            Echo($"Welder group '{_welderGroupName}' not found.");
        }
        else
        {
            group.GetBlocksOfType(_welders, w => WelderMatches(w, sensorGridId));
        }
    }
    else
    {
        GridTerminalSystem.GetBlocksOfType(_welders, w => WelderMatches(w, sensorGridId));
    }

    if (_welders.Count == 0)
    {
        Echo("No welders matched the filters.");
    }
}

bool BlockMatchesSensorFilter(IMySensorBlock sensor)
{
    if (!string.IsNullOrWhiteSpace(_sensorName) && !sensor.CustomName.Contains(_sensorName, StringComparison.OrdinalIgnoreCase))
        return false;
    if (_sameConstructOnly && !sensor.IsSameConstructAs(Me))
        return false;
    return true;
}

bool WelderMatches(IMyShipWelder welder, ulong sensorGridId)
{
    if (_sensor == null)
        return false;

    if (!string.IsNullOrWhiteSpace(_welderTag) && !welder.CustomName.Contains(_welderTag, StringComparison.OrdinalIgnoreCase))
        return false;

    if (_sameGridOnly && welder.CubeGrid.EntityId != sensorGridId)
        return false;

    if (_sameConstructOnly && !_sensor.IsSameConstructAs(welder))
        return false;

    return true;
}

void AlignSensor()
{
    if (!_configReady)
        return;

    if (_sensor == null)
    {
        Echo("Cannot align: sensor missing.");
        return;
    }

    if (_welders.Count == 0)
    {
        Echo("Cannot align: no welders matched.");
        ResetSensorExtents(_sensor);
        return;
    }

    Vector3D origin = _sensor.WorldMatrix.Translation;
    Vector3D forward = _sensor.WorldMatrix.Forward;
    Vector3D backward = _sensor.WorldMatrix.Backward;
    Vector3D up = _sensor.WorldMatrix.Up;
    Vector3D down = _sensor.WorldMatrix.Down;
    Vector3D right = _sensor.WorldMatrix.Right;
    Vector3D left = _sensor.WorldMatrix.Left;

    double front = 0, back = 0, top = 0, bottom = 0, leftExt = 0, rightExt = 0;
    double gridSize = _sensor.CubeGrid.GridSize;
    double padding = Math.Max(0.0, _padding);

    int blocksCounted = 0;
    foreach (var welder in _welders)
    {
        if (welder?.CubeGrid == null)
            continue;

        var slim = welder.CubeGrid.GetCubeBlock(welder.Position);
        if (slim == null)
        {
            ConsiderPoint(welder.GetPosition());
            continue;
        }

        Vector3I min = slim.Min;
        Vector3I max = slim.Max;
        double cellSize = welder.CubeGrid.GridSize;

        for (int xi = 0; xi < 2; xi++)
        {
            double x = (xi == 0 ? min.X : max.X + 1) - 0.5;
            for (int yi = 0; yi < 2; yi++)
            {
                double y = (yi == 0 ? min.Y : max.Y + 1) - 0.5;
                for (int zi = 0; zi < 2; zi++)
                {
                    double z = (zi == 0 ? min.Z : max.Z + 1) - 0.5;
                    Vector3D localCorner = new Vector3D(x, y, z) * cellSize;
                    Vector3D worldCorner = Vector3D.Transform(localCorner, welder.CubeGrid.WorldMatrix);
                    ConsiderPoint(worldCorner);
                }
            }
        }

        blocksCounted++;
    }

    // Apply padding and clamp against sensor capabilities
    float maxRange = _sensor.MaxRange;
    _sensor.FrontExtend = SafeClamp(front + padding, maxRange);
    _sensor.BackExtend = SafeClamp(back + padding, maxRange);
    _sensor.TopExtend = SafeClamp(top + padding, maxRange);
    _sensor.BottomExtend = SafeClamp(bottom + padding, maxRange);
    _sensor.LeftExtend = SafeClamp(leftExt + padding, maxRange);
    _sensor.RightExtend = SafeClamp(rightExt + padding, maxRange);

    Echo($"Sensor '{_sensor.CustomName}' updated.");
    Echo($"Matched welders: {_welders.Count} (processed {blocksCounted})");
    Echo($"Extents (m): F{_sensor.FrontExtend:0.00} / B{_sensor.BackExtend:0.00} / L{_sensor.LeftExtend:0.00} / R{_sensor.RightExtend:0.00} / T{_sensor.TopExtend:0.00} / Bt{_sensor.BottomExtend:0.00}");

    if (_debug)
    {
        Echo($"Sensor grid size: {gridSize:0.00} m");
        Echo($"Padding applied: {padding:0.00} m");
    }

    void ConsiderPoint(Vector3D worldPoint)
    {
        Vector3D delta = worldPoint - origin;

        double frontProj = Vector3D.Dot(delta, forward);
        if (frontProj > front) front = frontProj;

        double backProj = Vector3D.Dot(delta, backward);
        if (backProj > back) back = backProj;

        double topProj = Vector3D.Dot(delta, up);
        if (topProj > top) top = topProj;

        double bottomProj = Vector3D.Dot(delta, down);
        if (bottomProj > bottom) bottom = bottomProj;

        double rightProj = Vector3D.Dot(delta, right);
        if (rightProj > rightExt) rightExt = rightProj;

        double leftProj = Vector3D.Dot(delta, left);
        if (leftProj > leftExt) leftExt = leftProj;
    }
}

void ResetSensorExtents(IMySensorBlock sensor)
{
    sensor.FrontExtend = 0;
    sensor.BackExtend = 0;
    sensor.TopExtend = 0;
    sensor.BottomExtend = 0;
    sensor.LeftExtend = 0;
    sensor.RightExtend = 0;
}

float SafeClamp(double value, float maxRange)
{
    if (double.IsNaN(value) || double.IsInfinity(value))
        return 0f;
    value = Math.Max(0.0, value);
    value = Math.Min(maxRange, value);
    return (float)value;
}

void Echo(string text)
{
    base.Echo(text);
    _sb.AppendLine(text);
}

void EchoHelp()
{
    var sb = new StringBuilder();
    sb.AppendLine("Yezus Printer Helper - Sensor Matching");
    sb.AppendLine("Arguments:");
    sb.AppendLine("  (empty)   Run with current config");
    sb.AppendLine("  refresh   Reload blocks and update");
    sb.AppendLine("  scan      Alias for refresh");
    sb.AppendLine("  help      Show this help");
    sb.AppendLine();
    sb.AppendLine("Key Custom Data fields in [SensorSetup]:");
    sb.AppendLine("  SensorName     Substring match for the target sensor.");
    sb.AppendLine("  WelderGroup    Optional block group for welders.");
    sb.AppendLine("  WelderTag      Substring filter for welder names.");
    sb.AppendLine("  PaddingMeters  Extra buffer beyond welder bounds.");
    sb.AppendLine("  AutoUpdate     true -> recalc every 100 ticks.");
    sb.AppendLine("  OnlySensorGrid Restrict welders to sensor grid.");
    sb.AppendLine("  SameConstruct  Require welders on same construct.");
    sb.AppendLine("  Debug          Echo extra diagnostics.");
    Echo(sb.ToString());
}

void ConfigureSurface(IMyTextSurface surface)
{
    if (surface == null)
        return;

    surface.ContentType = ContentType.TEXT_AND_IMAGE;
    surface.Font = "Monospace";
    surface.Alignment = TextAlignment.LEFT;
}

void BeginStatusFrame(string argument, UpdateType updateSource)
{
    _sb.Clear();
    _sb.AppendLine("Yezus Printer Helper");
    _sb.AppendLine("--------------------");
    _sb.AppendLine($"Command: {argument}");
    _sb.AppendLine($"Update: {updateSource}");
    _sb.AppendLine();
}

void WriteStatusSurface()
{
    if (_statusSurface == null)
        return;

    if (_sb.Length == 0)
    {
        _statusSurface.WriteText("Yezus Printer Helper\nReady.");
    }
    else
    {
        _statusSurface.WriteText(_sb, false);
    }
}
