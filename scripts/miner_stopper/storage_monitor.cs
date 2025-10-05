// === QG Storage Monitor v7.1 ===
// Stops or resumes PAM miners depending on [QG] storage level
// Retries up to 3 times if miners not yet visible after connector lock

// ---------- CONFIG ----------
const string TAG_QG = "[QG]";
const double THRESHOLD_PERCENT = 75.0;

const int RETRY_COUNT = 3;
const int RETRY_DELAY_TICKS = 60; // ~1s @60TPS

string[] EXCLUDED_KEYWORDS = { "Special", "Locked", "Seat", "Control Station" };

string[] MINER_BLOCKS = {
    "[HMiner-1] Programmable Block",
    "[HMiner-2] Programmable Block"
};
// --------------------------------

// Retry state
int attemptsDone = 0;
int delayTicks = 0;
bool retryPending = false;

void Main(string argument, UpdateType updateSource)
{
    string now = DateTime.Now.ToString("HH:mm:ss");

    // Tick-driven retry loop
    if ((updateSource & UpdateType.Update1) != 0 && retryPending)
    {
        if (--delayTicks <= 0)
        {
            bool done = RunOnce(now);
            if (!done && attemptsDone < RETRY_COUNT)
            {
                attemptsDone++;
                delayTicks = RETRY_DELAY_TICKS;
                Echo($"[{now}] Retry {attemptsDone}/{RETRY_COUNT} in {(RETRY_DELAY_TICKS/60.0):0.0}s");
            }
            else
            {
                if (!done) Echo($"[{now}] Gave up after {attemptsDone} retries.");
                StopRetry();
            }
        }
        return;
    }

    // Initial trigger
    attemptsDone = 0;
    bool ok = RunOnce(now);
    if (!ok)
    {
        retryPending = true;
        attemptsDone = 1;
        delayTicks = RETRY_DELAY_TICKS;
        Runtime.UpdateFrequency = UpdateFrequency.Update1;
        Echo($"[{now}] Retry {attemptsDone}/{RETRY_COUNT} in {(RETRY_DELAY_TICKS/60.0):0.0}s");
    }
}

bool RunOnce(string now)
{
    // 1) Collect QG containers
    var containers = new List<IMyCargoContainer>();
    GridTerminalSystem.GetBlocksOfType(containers, c =>
        c.CustomName.IndexOf(TAG_QG, StringComparison.OrdinalIgnoreCase) >= 0 &&
        !ContainsAny(c.CustomName, EXCLUDED_KEYWORDS)
    );

    double totalCap = 0, totalUsed = 0;
    foreach (var c in containers)
    {
        var inv = c.GetInventory();
        totalCap  += (double)inv.MaxVolume;
        totalUsed += (double)inv.CurrentVolume;
    }

    if (totalCap <= 0)
    {
        Echo($"[{now}] No valid {TAG_QG} containers found.");
        return true; // rien à réessayer
    }

    double percentUsed = (totalUsed / totalCap) * 100.0;
    Echo($"[{now}] QG Storage: {percentUsed:0.0}% full");

    // 2) STOP si >= seuil, sinon CONT
    if (percentUsed >= THRESHOLD_PERCENT)
        return ExecuteCommand("STOP", now);
    else
        return ExecuteCommand("CONT", now);
}

bool ExecuteCommand(string cmd, string now)
{
    Echo($"[{now}] Sending {cmd} command to miners...");
    bool missing = false;

    foreach (var minerName in MINER_BLOCKS)
    {
        var minerPB = GridTerminalSystem.GetBlockWithName(minerName) as IMyProgrammableBlock;
        if (minerPB == null)
        {
            Echo($"[{now}] Not found: {minerName}");
            missing = true; // on retentera si au moins un est manquant
            continue;
        }

        bool ok = minerPB.TryRun(cmd);
        Echo(ok
            ? $"[{now}] Sent {cmd} to {minerName}"
            : $"[{now}] ERROR: TryRun failed on {minerName}");
    }

    return !missing; // retry uniquement si un PB était introuvable
}

bool ContainsAny(string name, string[] keywords)
{
    for (int i = 0; i < keywords.Length; i++)
        if (name.IndexOf(keywords[i], StringComparison.OrdinalIgnoreCase) >= 0)
            return true;
    return false;
}

void StopRetry()
{
    retryPending = false;
    attemptsDone = 0;
    delayTicks = 0;
    Runtime.UpdateFrequency = UpdateFrequency.None;
}
