// Built-in locomotive dashboard (driver's cab window) of Rigs of Rods, drawn with DearIMGUI from AngelScript.
// Loaded by 'imgui_train.dashboard' (a dashboard without '.layout' file) for '.train' actors, see
// DashBoardManager::loadDashBoard() and the 'Default train dashboard' setting.
// Throttle and train brake levers (stay where the player left them; the keyboard takes over as soon as it is used),
// direction, engine RPM, speed limit, horn, parking brake, light/prop switches, couplings and command keys.
// ---------------------------------------------------------------------------------------------------------------

color Col(int r, int g, int b, int a = 255) { return color(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }

const color COL_BAR_BG      = Col( 18,  19,  21);
const color COL_THROTTLE    = Col( 55, 138, 221);
const color COL_BRAKE       = Col(226,  75,  74);
const color COL_GRAB        = Col(208, 211, 216);
const color COL_GRAB_EDGE_T = Col(133, 183, 235);
const color COL_GRAB_EDGE_B = Col(240, 149, 149);
const color COL_DIAL_BG     = Col( 18,  19,  21);
const color COL_DIAL_RIM    = Col( 90,  93,  99);
const color COL_DIAL_TRACK  = Col( 60,  63,  68);
const color COL_DIAL_OK     = Col( 79, 160,  90);
const color COL_DIAL_OVER   = Col(226,  75,  74);
const color COL_TEXT        = Col(230, 230, 230);
const color COL_MUTED       = Col(154, 157, 163);
const color COL_BTN_BLUE    = Col( 43,  79, 128);
const color COL_BTN_RED     = Col(138,  48,  48);
const color COL_BTN_GREY    = Col( 58,  61,  66);
const color COL_BTN_ON      = Col( 46, 107,  58);
const color COL_BTN_DIM     = Col( 36,  38,  42);

const float BASE_WIDTH = 300.f; // px at ui_scale_factor 1.0
const float PI_F = 3.14159265f;
const int   MAX_COMMANDS = 84;
const int   MAX_LIGHT_SWITCHES = 10; // TRUCK_LIGHTTOGGLE01..10

float fmin(float a, float b) { return (a < b) ? a : b; }
float fmax(float a, float b) { return (a > b) ? a : b; }
float clampf(float v, float lo, float hi) { return fmin(fmax(v, lo), hi); }

color Lighten(const color&in c, int d)
{
    return color(fmin(1.f, c.r + d / 255.f), fmin(1.f, c.g + d / 255.f), fmin(1.f, c.b + d / 255.f), c.a);
}

/// ImGui::Text() and friends use a format string: keep '%' as it is
string Esc(const string&in s)
{
    string r;
    for (uint i = 0; i < s.length(); i++)
    {
        r += s.substr(i, 1);
        if (s[i] == 37) // '%'
            r += "%";
    }
    return r;
}

inputEvents CommandEvent(int key) // commandkeys are indexed 1-MAX_COMMANDS
{
    return inputEvents(int(EV_COMMANDS_01) + (key - 1));
}

// ---- State ----

bool  g_attached = false;          // the levers drive this locomotive's inputs
float g_throttle = 0.f;            // 0..1, stays where the player left it
float g_train_brake = 0.f;         // 0..1, stays where the player left it
array<int> g_pulses;               // events released at the start of the next frame
array<int> g_held;                 // events held by buttons in the previous frame
array<int> g_held_now;             // events held by buttons in this frame
bool  g_closed = false;            // the window was closed (for this locomotive)
float g_scale = 1.f;

bool Contains(const array<int>&in a, int v)
{
    for (uint i = 0; i < a.length(); i++)
    {
        if (a[i] == v)
            return true;
    }
    return false;
}

/// Hand all simulated inputs back to the player
void Detach()
{
    if (g_attached)
    {
        thisActor.resetEventSimulatedValue(EV_TRUCK_ACCELERATE);
        thisActor.resetEventSimulatedValue(EV_TRUCK_BRAKE);
        for (uint i = 0; i < g_held.length(); i++)
            thisActor.resetEventSimulatedValue(inputEvents(g_held[i]));
        for (uint i = 0; i < g_pulses.length(); i++)
            thisActor.resetEventSimulatedValue(inputEvents(g_pulses[i]));
    }
    g_attached = false;
    g_throttle = 0.f;
    g_train_brake = 0.f;
    g_held.resize(0);
    g_pulses.resize(0);
}

void ApplyLevers()
{
    // Keyboard/joystick wins: as soon as it is used, the lever drops back to 0.
    if (inputs.getEventValue(EV_TRUCK_ACCELERATE) > 0.02f)
        g_throttle = 0.f;
    if (inputs.getEventValue(EV_TRUCK_BRAKE) > 0.02f)
        g_train_brake = 0.f;

    if (g_throttle > 0.f)
        thisActor.setEventSimulatedValue(EV_TRUCK_ACCELERATE, g_throttle);
    else
        thisActor.resetEventSimulatedValue(EV_TRUCK_ACCELERATE);

    if (g_train_brake > 0.f)
        thisActor.setEventSimulatedValue(EV_TRUCK_BRAKE, g_train_brake);
    else
        thisActor.resetEventSimulatedValue(EV_TRUCK_BRAKE);
}

bool ShouldDraw()
{
    if (thisActor is null || thisActor.getEngine() is null) // wagons have no engine
        return false;
    BeamClass@ player = game.getCurrentTruck();
    if (player is null || player.getInstanceId() != thisActor.getInstanceId())
        return false;
    const TruckState st = thisActor.getTruckState();
    if (st != TS_SIMULATED && st != TS_SLEEPING)
        return false;
    // This is a control desk, not a gauge: unlike the other dashboards it stays visible in the cab view (cinecam),
    // where the game hides the screen dashboards. It is hidden only with the whole GUI; the X button closes it.
    CVarClass@ hide_gui = console.cVarFind("ui_hide_gui");
    return hide_gui is null || !hide_gui.getBool();
}

// Required even though it's empty: ScriptEngine::setupScriptUnit() only sets `thisActor` when the script has a `main()`.
void main()
{
}

void frameStep(float dt)
{
    // Player left the locomotive, switched vehicles or hid the dashboard: give the controls back.
    if (!ShouldDraw())
    {
        Detach();
        return;
    }
    g_attached = true;

    // Toggles from the previous frame are released now.
    for (uint i = 0; i < g_pulses.length(); i++)
        thisActor.resetEventSimulatedValue(inputEvents(g_pulses[i]));
    g_pulses.resize(0);

    if (g_closed)
    {
        g_throttle = 0.f;
        g_train_brake = 0.f;
        ApplyLevers();
        return;
    }

    ApplyLevers();

    CVarClass@ ui_scale = console.cVarFind("ui_scale_factor");
    g_scale = (ui_scale !is null) ? ui_scale.getFloat() : 1.f;
    ImGui::SetNextWindowPos(vector2(20.f, 200.f), ImGuiCond_FirstUseEver);
    bool open = true;
    if (ImGui::Begin(thisActor.getTruckName() + "###TrainPanel", open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
    {
        g_held_now.resize(0);

        DrawStatus();
        ImGui::Dummy(vector2(0.f, 4.f * g_scale));

        // Show the keyboard value while a key is held (keyboard wins, see ApplyLevers()).
        const float key_accel = inputs.getEventValue(EV_TRUCK_ACCELERATE);
        const float key_brake = inputs.getEventValue(EV_TRUCK_BRAKE);
        g_throttle = DrawBar("##throttle", "Throttle", g_throttle, fmax(g_throttle, key_accel), COL_THROTTLE, COL_GRAB_EDGE_T);
        ImGui::Dummy(vector2(0.f, 2.f * g_scale));
        g_train_brake = DrawBar("##trainbrake", "Train brake", g_train_brake, fmax(g_train_brake, key_brake), COL_BRAKE, COL_GRAB_EDGE_B);

        ImGui::Dummy(vector2(0.f, 2.f * g_scale));
        DrawDirection();

        ImGui::Separator();
        DrawSwitches();
        DrawCouplings();
        DrawCommands();

        // Minimum width only - the window grows with its content (long names, big fonts)
        ImGui::Dummy(vector2(BASE_WIDTH * g_scale, 0.f));

        // Buttons released this frame: hand the event back to the keyboard.
        for (uint i = 0; i < g_held.length(); i++)
        {
            if (!Contains(g_held_now, g_held[i]))
                thisActor.resetEventSimulatedValue(inputEvents(g_held[i]));
        }
        g_held = g_held_now;
    }
    ImGui::End();

    if (!open)
    {
        g_closed = true;
        g_throttle = 0.f;
        g_train_brake = 0.f;
        ApplyLevers();
    }
}

void DrawStatus()
{
    const float kmh = thisActor.getSpeed() * 3.6f;
    const float limit_kmh = thisActor.getSpeedLimiterEnabled() ? thisActor.getSpeedLimit() * 3.6f : 0.f;

    DrawTacho(kmh, limit_kmh);
    ImGui::SameLine();

    ImGui::BeginGroup();
    string dir = "neutral";
    float rpm = 0.f;
    EngineClass@ engine = thisActor.getEngine();
    if (engine !is null)
    {
        const int gear = engine.getGear();
        if (gear > 0)
            dir = "forward";
        else if (gear < 0)
            dir = "reverse";
        rpm = engine.getRPM();
    }
    // Value column right of the widest label
    const float label_w = fmax(fmax(ImGui::CalcTextSize("Direction").x, ImGui::CalcTextSize("Engine").x), ImGui::CalcTextSize("Limit").x);
    const float col2 = ImGui::GetCursorPosX() + label_w + 16.f * g_scale;
    ImGui::TextColored(COL_MUTED, "Direction"); ImGui::SameLine(col2); ImGui::Text(dir);
    ImGui::TextColored(COL_MUTED, "Engine");    ImGui::SameLine(col2); ImGui::Text(formatFloat(rpm, "", 0, 0) + " rpm");
    ImGui::TextColored(COL_MUTED, "Limit");     ImGui::SameLine(col2);
    if (limit_kmh > 0.f)
        ImGui::Text(formatFloat(limit_kmh, "", 0, 0) + " km/h");
    else
        ImGui::Text("off");
    ImGui::EndGroup();
}

void DrawTacho(float kmh, float limit_kmh)
{
    const float size = 92.f * g_scale;
    const vector2 p0 = ImGui::GetCursorScreenPos();
    ImGui::Dummy(vector2(size, size));
    ImDrawList@ dl = ImGui::GetWindowDrawList();

    // Scale end: next multiple of 20 above 1.2x the limit (or 120 without limit)
    float max_kmh = (limit_kmh > 0.f) ? ceil(limit_kmh * 1.2f / 20.f) * 20.f : 120.f;
    max_kmh = fmax(max_kmh, ceil(kmh / 20.f) * 20.f);
    max_kmh = fmax(max_kmh, 20.f);

    const vector2 c(p0.x + size * 0.5f, p0.y + size * 0.5f);
    const float r = size * 0.45f;
    dl.AddCircleFilled(c, r, COL_DIAL_BG, 32);
    dl.AddCircle(c, r, COL_DIAL_RIM, 32, 2.f);

    // 270 degree arc from bottom-left (135 deg) clockwise to bottom-right (405 deg), screen coords
    const float a0 = 135.f * PI_F / 180.f;
    const float sweep = 270.f * PI_F / 180.f;
    const float frac = clampf(kmh / max_kmh, 0.f, 1.f);
    const float track_r = r - 6.f * g_scale;
    const float thick = 5.f * g_scale;

    dl.PathClear();
    dl.PathArcTo(c, track_r, a0, a0 + sweep, 32);
    dl.PathStroke(COL_DIAL_TRACK, false, thick);
    if (frac > 0.f)
    {
        const bool over = (limit_kmh > 0.f) && (kmh > limit_kmh);
        dl.PathArcTo(c, track_r, a0, a0 + sweep * frac, 32);
        dl.PathStroke(over ? COL_DIAL_OVER : COL_DIAL_OK, false, thick);
    }
    if (limit_kmh > 0.f) // limit tick
    {
        const float al = a0 + sweep * clampf(limit_kmh / max_kmh, 0.f, 1.f);
        dl.AddLine(vector2(c.x + cos(al) * (r - 12.f * g_scale), c.y + sin(al) * (r - 12.f * g_scale)),
                   vector2(c.x + cos(al) * r, c.y + sin(al) * r), COL_DIAL_OVER, 2.f);
    }

    const float an = a0 + sweep * frac;
    dl.AddLine(c, vector2(c.x + cos(an) * (r - 10.f * g_scale), c.y + sin(an) * (r - 10.f * g_scale)), COL_DIAL_OVER, 3.f * g_scale);
    dl.AddCircleFilled(c, 4.f * g_scale, COL_TEXT, 12);

    const string val = formatFloat(kmh, "", 0, 0);
    const vector2 vs = ImGui::CalcTextSize(val);
    dl.AddText(vector2(c.x - vs.x * 0.5f, c.y + 10.f * g_scale), COL_TEXT, val);
    const vector2 us = ImGui::CalcTextSize("km/h");
    dl.AddText(vector2(c.x - us.x * 0.5f, c.y + 10.f * g_scale + vs.y), COL_MUTED, "km/h");
}

/// Horizontal lever; returns the new value
float DrawBar(const string&in id, const string&in label, float value, float shown, const color&in fill, const color&in grab_edge)
{
    ImGui::TextColored(COL_MUTED, label);
    const string pct = formatFloat(shown * 100.f, "", 0, 0) + " %";
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(pct).x);
    ImGui::Text(Esc(pct));

    const float w = ImGui::GetContentRegionAvail().x;
    const float h = 10.f * g_scale;
    const float grab_w = 8.f * g_scale;
    const float pad = 4.f * g_scale;
    const vector2 cur = ImGui::GetCursorScreenPos();
    const vector2 p0(cur.x, cur.y + pad);
    const vector2 p1(p0.x + w, p0.y + h);

    // Hit area slightly taller than the bar so the grab is easy to catch
    ImGui::InvisibleButton(id, vector2(w, h + 2.f * pad));
    if (ImGui::IsItemActive())
    {
        float v = clampf((ImGui::GetMousePos().x - p0.x) / w, 0.f, 1.f);
        if (v < 0.01f)
            v = 0.f; // far left snaps to "off"
        value = v;
        shown = v;
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Drag the slider with the mouse. Far left = off, then the keyboard takes over again.");

    ImDrawList@ dl = ImGui::GetWindowDrawList();
    dl.AddRectFilled(p0, p1, COL_BAR_BG, 2.f);
    if (shown > 0.f)
        dl.AddRectFilled(p0, vector2(p0.x + w * shown, p1.y), fill, 2.f);

    const float gx = clampf(p0.x + w * shown - grab_w * 0.5f, p0.x, p1.x - grab_w);
    const vector2 g0(gx, p0.y - pad);
    const vector2 g1(gx + grab_w, p1.y + pad);
    dl.AddRectFilled(g0, g1, COL_GRAB, 2.f);
    dl.AddRect(g0, g1, grab_edge, 2.f);
    return value;
}

bool ColoredButton(const string&in label, const vector2&in size, const color&in col)
{
    ImGui::PushStyleColor(ImGuiCol_Button, col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Lighten(col, 20));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Lighten(col, 38));
    const bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return pressed;
}

void HoldButton(const string&in label, inputEvents ev, const vector2&in size, const color&in col)
{
    ColoredButton(label, size, col);
    if (ImGui::IsItemActive())
    {
        thisActor.setEventSimulatedValue(ev, 1.f);
        g_held_now.insertLast(int(ev));
    }
}

/// Toggle: event active for exactly one frame
void Pulse(inputEvents ev)
{
    thisActor.setEventSimulatedValue(ev, 1.f);
    g_pulses.insertLast(int(ev));
}

void KeyTooltip(inputEvents ev)
{
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(Esc("Key: " + inputs.getEventCommandTrimmed(ev)));
}

int g_toggle_col = 0;
void DrawToggle(const string&in label, bool on, inputEvents ev, const vector2&in half_size)
{
    if (g_toggle_col % 2 == 1)
        ImGui::SameLine();
    if (ColoredButton(label, half_size, on ? COL_BTN_ON : COL_BTN_GREY))
        Pulse(ev);
    KeyTooltip(ev);
    g_toggle_col++;
}

/// Direction selector: reverse / neutral / forward.
/// Like a real locomotive, the direction can only be reversed at (almost) standstill.
void DrawDirection()
{
    EngineClass@ engine = thisActor.getEngine();
    if (engine is null)
        return;

    const bool automatic = (engine.getAutoMode() == AUTO);
    int cur = 0; // -1 reverse, 0 neutral, 1 forward
    if (automatic)
    {
        const autoswitch sel = engine.getAutoShift();
        cur = (sel == AUTOSWITCH_REAR) ? -1 : ((sel == AUTOSWITCH_NEUTRAL) ? 0 : 1);
    }
    else
    {
        const int gear = engine.getGear();
        cur = (gear < 0) ? -1 : ((gear > 0) ? 1 : 0);
    }
    const bool moving = abs(thisActor.getSpeed()) > 0.5f; // m/s

    ImGui::TextColored(COL_MUTED, "Direction");
    const float third = (ImGui::GetContentRegionAvail().x - 16.f * g_scale) / 3.f;
    const array<string> labels = { "Reverse", "Neutral", "Forward" };
    for (int i = 0; i < 3; i++)
    {
        const int want = i - 1;
        if (i > 0)
            ImGui::SameLine();
        const bool active = (want == cur);
        const bool blocked = moving && (want * cur < 0); // reversing while moving
        const color col = active ? COL_BTN_ON : (blocked ? COL_BTN_DIM : COL_BTN_GREY);
        if (ColoredButton(labels[i] + "##dir" + i, vector2(third, 0.f), col) && !active && !blocked)
        {
            if (automatic)
                engine.autoShiftSet((want < 0) ? AUTOSWITCH_REAR : ((want == 0) ? AUTOSWITCH_NEUTRAL : AUTOSWITCH_DRIVE));
            else
                engine.shiftTo(want);
        }
        if (blocked && ImGui::IsItemHovered())
            ImGui::SetTooltip("Stop the train first");
    }
}

void DrawSwitches()
{
    const float half = (ImGui::GetContentRegionAvail().x - 8.f * g_scale) * 0.5f;
    const vector2 half_size(half, 0.f);

    HoldButton("Horn", EV_TRUCK_HORN, half_size, COL_BTN_GREY);
    ImGui::SameLine();
    const bool pb = thisActor.getParkingBrake();
    if (ColoredButton(pb ? "Parking brake on" : "Parking brake off", half_size, pb ? COL_BTN_RED : COL_BTN_GREY))
        thisActor.parkingbrakeToggle();

    g_toggle_col = 0;
    // Switchable lights/props: custom light flares (u-type) and toggle-props on TRUCK_LIGHTTOGGLE01..10
    for (int n = 0; n < MAX_LIGHT_SWITCHES; n++)
    {
        const inputEvents ev = inputEvents(int(EV_TRUCK_LIGHTTOGGLE01) + n);
        bool used = false;
        bool on = false;
        if (thisActor.countCustomLights(n) > 0)
        {
            used = true;
            on = thisActor.getCustomLightVisible(n);
        }
        const int prop_state = thisActor.getPropAnimKeyState(ev);
        if (prop_state >= 0)
        {
            if (!used)
                on = (prop_state == 1);
            used = true;
        }
        if (used)
            DrawToggle("Light " + (n + 1) + (on ? " on" : "") + "###light" + n, on, ev, half_size);
    }

    // Toggle-props on command keys without command beams (e.g. wipers on COMMANDS_15)
    for (int key = 1; key <= MAX_COMMANDS; key++)
    {
        const int prop_state = thisActor.getPropAnimKeyState(CommandEvent(key));
        if (prop_state < 0 || thisActor.isCommandKeyInUse(key))
            continue; // no toggle-prop, or a real command (listed below)
        const bool on = (prop_state == 1);
        DrawToggle("Switch " + key + (on ? " on" : "") + "###cmdsw" + key, on, CommandEvent(key), half_size);
    }
}

void DrawCouplings()
{
    const int num_hooks = thisActor.getHookCount();
    if (num_hooks == 0)
        return;

    ImGui::Separator();
    ImGui::TextColored(COL_MUTED, "Couplings");

    const vector3 dir = thisActor.getDirection();
    const vector3 center = thisActor.getPosition();
    const float btn_w = 90.f * g_scale;

    for (int i = 0; i < num_hooks; i++)
    {
        const int lock_state = thisActor.getHookLockState(i);
        if (lock_state < 0)
            continue;

        const int hook_node = thisActor.getHookNode(i);
        if (hook_node < 0)
            continue;

        const bool front = (thisActor.getNodePosition(hook_node) - center).dotProduct(dir) > 0.f;
        string state;
        if (lock_state == 2) // LOCKED
        {
            const string other = thisActor.getHookLockedActorName(i);
            state = (other != "") ? "coupled (" + other + ")" : "coupled";
        }
        else if (lock_state == 1) // PRELOCK
        {
            state = "locking";
        }
        else
        {
            state = "open";
        }

        ImGui::PushID(i + 1);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(front ? "front" : "rear");
        ImGui::SameLine();
        ImGui::TextColored(COL_MUTED, Esc(state));
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - btn_w);
        const bool locked = (lock_state != 0);
        if (ColoredButton(locked ? "Uncouple" : "Couple", vector2(btn_w, 0.f), locked ? COL_BTN_RED : COL_BTN_BLUE))
        {
            // Same as clicking the hook node with the mouse
            game.pushMessage(MSG_SIM_ACTOR_LINKING_REQUESTED, {
                {'instance_id', thisActor.getInstanceId()},
                {'type', ACTORLINKINGREQUESTTYPE_HOOK_MOUSE_TOGGLE},
                {'hook_group', thisActor.getHookGroup(i)},
                {'hook_mousenode', hook_node}
            });
        }
        ImGui::PopID();
    }
}

void DrawCommands()
{
    const int num_pairs = thisActor.getCommandKeyPairCount();
    if (num_pairs == 0)
        return;

    ImGui::Separator();
    ImGui::TextColored(COL_MUTED, "Commands (hold to operate)");

    const vector2 cmd_btn_size(28.f * g_scale, 0.f);
    for (int i = 0; i < num_pairs; i++)
    {
        const int key1 = thisActor.getCommandKeyPairKey1(i);
        const int key2 = thisActor.getCommandKeyPairKey2(i);
        string desc = thisActor.getCommandKeyPairDescription(i);
        for (uint j = 0; j < desc.length(); j++)
        {
            if (desc[j] == 95) // '_'
                desc[j] = 32;  // ' '
        }
        if (desc == "")
            desc = "Command " + key1 + "/" + key2;

        ImGui::PushID(i + 1);
        ImGui::AlignTextToFramePadding();
        ImGui::Text(Esc(desc));
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - cmd_btn_size.x * 2.f - 8.f * g_scale);
        HoldButton("-", CommandEvent(key1), cmd_btn_size, COL_BTN_GREY);
        KeyTooltip(CommandEvent(key1));
        ImGui::SameLine();
        HoldButton("+", CommandEvent(key2), cmd_btn_size, COL_BTN_GREY);
        KeyTooltip(CommandEvent(key2));
        ImGui::PopID();
    }
}
