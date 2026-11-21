// Built-in car/truck dashboard of Rigs of Rods, drawn with DearIMGUI from AngelScript.
// Loaded by 'imgui_car.dashboard' (a dashboard without '.layout' file), see DashBoardManager::loadDashBoard();
// choose it in Settings > UI > Default truck dashboard.
// Three views (settings button): one combined gauge, two gauges (+ boost and tilt) or the full panel
// (speed, RPM, gearbox, driving aids & engine, lights, tyres & drive). Every part can be hidden.
// Adapts itself to the vehicle: the RPM scale follows the shift-up RPM (diesel trucks get a finer scale),
// electric engines get a power gauge instead of the RPM gauge, the boost gauge is only shown with a turbo.
// Units follow 'Imperial units' (Settings > Gameplay) unless set otherwise; texts follow the game language.
// ---------------------------------------------------------------------------------------------------------------

const float PI_F = 3.14159265f;
const float DEG2RAD = PI_F / 180.f;
const float MS_TO_KMH = 3.6f;
const float MS_TO_MPH = 2.23694f;
const float PSI_TO_BAR = 0.0689476f;
const int   MAX_LIGHT_SWITCHES = 10; // TRUCK_LIGHTTOGGLE01..10

color Col(int r, int g, int b, int a = 255) { return color(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }

const color COL_DIAL_DEF = Col(13, 18, 23);   // default instrument background
const int   BG_PCT_DEF   = 90;                // default background opacity [%]
const color COL_DIAL_BR_DEF = Col(255, 255, 255, 46);
const color COL_CARD    = Col(22, 27, 34);
const color COL_CARD_BR = Col(42, 49, 58);
const color COL_TRACK   = Col(42, 49, 58);
const color COL_TEXT    = Col(245, 247, 250);
const color COL_TICK    = Col(201, 209, 217);
const color COL_MUTED   = Col(157, 167, 179);
const color COL_DARK    = Col(58, 68, 80);
const color COL_INK     = Col(11, 16, 22);
const color COL_ACCENT  = Col(63, 184, 232);
const color COL_WARN    = Col(240, 160, 32);
const color COL_RED     = Col(229, 83, 75);
const color COL_GREEN   = Col(63, 185, 80);
const color COL_BTN     = Col(33, 38, 45);
const color COL_BTN_HOV = Col(48, 55, 64);
const color COL_GREEN_BG= Col(46, 107, 58);
const color COL_WARN_BG = Col(74, 52, 16);

float fmin(float a, float b) { return (a < b) ? a : b; }
float fmax(float a, float b) { return (a > b) ? a : b; }
float clampf(float v, float lo, float hi) { return fmin(fmax(v, lo), hi); }
int   round_i(float v) { return int(floor(v + 0.5f)); }

/// ImGui::SetTooltip() and friends use a format string: keep '%' as it is
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

// ---- Texts (English and German; other languages use English) ----

enum Txt
{
    TX_SPEED, TX_RPM, TX_RPM_UNIT, TX_RPM_X1000, TX_RPM_X100, TX_POWER, TX_REGEN, TX_BOOST, TX_PITCH, TX_ROLL,
    TX_GEARBOX, TX_AIDS, TX_LIGHTS, TX_TYRES, TX_CRUISE, TX_CRUISE_OFF, TX_LIMITER, TX_LIMITER_OFF,
    TX_ABS, TX_ABS_OFF, TX_TC, TX_TC_OFF, TX_PBRAKE, TX_ENGINE_ON, TX_ENGINE_OFF,
    TX_DIFF, TX_AXLE_DIFF, TX_DIFF_SPLIT, TX_DIFF_OPEN, TX_DIFF_VISCOUS, TX_DIFF_LOCKED,
    TX_LOW_BEAM, TX_HIGH_BEAM, TX_FOG, TX_BEACONS, TX_LIGHT_N, TX_RANGE,
    TX_MODE_AUTO, TX_MODE_SEMI, TX_MODE_MANUAL, TX_MODE_STICK, TX_MODE_RANGES,
    TX_PRESSURE, TX_NO_PRESSURE, TX_SLIP, TX_DRIVE_4WD, TX_DRIVE_2WD,
    TX_FULL_PANEL, TX_SETTINGS, TX_VIEW, TX_VIEW_ONE, TX_VIEW_TWO, TX_VIEW_FULL, TX_SHOW,
    TX_SH_SPEED, TX_SH_RPM_RING, TX_SH_RPM, TX_SH_CRUISE, TX_SH_BOOST, TX_SH_TURBO, TX_SH_PITCH, TX_SH_ROLL,
    TX_SH_GEARSTRIP, TX_SH_LAMPS, TX_SH_VIEWBTN, TX_SH_GEARBOX, TX_SH_AIDS, TX_SH_LIGHTS, TX_SH_TYRES,
    TX_COLORS, TX_COL_SPEED, TX_COL_RPM, TX_COL_RESET,
    TX_COL_BG, TX_BG_OPACITY, TX_UNITS, TX_UNITS_GAME, TX_SIZE, TX_ARRANGE, TX_ARRANGE_DONE, TX_ARRANGE_DONE_TIP, TX_SNAP, TX_ARRANGE_HINT, TX_RESET_LAYOUT, TX_OTHER_DASH, TX_KEY,
    TX_TIP_SETTINGS, TX_TIP_FULL, TX_TIP_SMALL, TX_TIP_LIMITER, TX_TIP_HOLD, TX_TIP_ENGINE,
    TX_COUNT
}

array<string> g_txt(TX_COUNT);

void S(Txt id, const string&in en, const string&in de, bool german)
{
    g_txt[id] = german ? de : en;
}

void InitTexts()
{
    string lang = "en";
    CVarClass@ cv = console.cVarFind("app_language");
    if (cv !is null && cv.getStr().length() >= 2)
        lang = cv.getStr().substr(0, 2);
    const bool d = (lang == "de");
    S(TX_SPEED, "Speed", "Geschwindigkeit", d);
    S(TX_RPM, "RPM", "Drehzahl", d);
    S(TX_RPM_UNIT, "rpm", "U/min", d);
    S(TX_RPM_X1000, "x1000 rpm", "\u00d71000 U/min", d);
    S(TX_RPM_X100, "x100 rpm", "\u00d7100 U/min", d);
    S(TX_POWER, "Power", "Leistung", d);
    S(TX_REGEN, "Regen", "Rekuperation", d);
    S(TX_BOOST, "Boost", "Ladedruck", d);
    S(TX_PITCH, "pitch", "l\u00e4ngs", d);
    S(TX_ROLL, "roll", "quer", d);
    S(TX_GEARBOX, "Gearbox", "Getriebe", d);
    S(TX_AIDS, "Driving aids & engine", "Fahrhilfen & Motor", d);
    S(TX_LIGHTS, "Lights", "Licht", d);
    S(TX_TYRES, "Tyres & drive", "Reifen & Antrieb", d);
    S(TX_CRUISE, "Cruise", "Tempomat", d);
    S(TX_CRUISE_OFF, "Cruise off", "Tempomat aus", d);
    S(TX_LIMITER, "Limiter", "Begrenzer", d);
    S(TX_LIMITER_OFF, "Limiter off", "Begrenzer aus", d);
    S(TX_ABS, "ABS", "ABS", d);
    S(TX_ABS_OFF, "ABS off", "ABS aus", d);
    S(TX_TC, "TC", "TC", d);
    S(TX_TC_OFF, "TC off", "TC aus", d);
    S(TX_PBRAKE, "Parking brake", "Feststellbremse", d);
    S(TX_ENGINE_ON, "Engine on", "Motor an", d);
    S(TX_ENGINE_OFF, "Engine off", "Motor aus", d);
    S(TX_DIFF, "Diff", "Diff", d);
    S(TX_AXLE_DIFF, "Axle diff", "L\u00e4ngsdiff", d);
    S(TX_DIFF_SPLIT, "split", "geteilt", d);
    S(TX_DIFF_OPEN, "open", "offen", d);
    S(TX_DIFF_VISCOUS, "viscous", "Visko", d);
    S(TX_DIFF_LOCKED, "locked", "gesperrt", d);
    S(TX_LOW_BEAM, "Low beam", "Abblend", d);
    S(TX_HIGH_BEAM, "High beam", "Fernlicht", d);
    S(TX_FOG, "Fog", "Nebel", d);
    S(TX_BEACONS, "Beacons", "Rundum", d);
    S(TX_LIGHT_N, "Light", "Licht", d);
    S(TX_RANGE, "Range", "Bereich", d);
    S(TX_MODE_AUTO, "Automatic", "Automatik", d);
    S(TX_MODE_SEMI, "Semi-auto", "Halbautomatik", d);
    S(TX_MODE_MANUAL, "Sequential", "Sequenziell", d);
    S(TX_MODE_STICK, "H-shifter", "H-Schaltung", d);
    S(TX_MODE_RANGES, "H-shifter + ranges", "H-Schaltung + Bereiche", d);
    S(TX_PRESSURE, "Tyre pressure", "Reifendruck", d);
    S(TX_NO_PRESSURE, "not adjustable", "nicht verstellbar", d);
    S(TX_SLIP, "Wheel slip", "Schlupf", d);
    S(TX_DRIVE_4WD, "4WD", "Allrad", d);
    S(TX_DRIVE_2WD, "2WD", "Zweirad", d);
    S(TX_FULL_PANEL, "Full panel", "Volles Panel", d);
    S(TX_SETTINGS, "Car dashboard", "Auto-Dashboard", d);
    S(TX_VIEW, "View", "Ansicht", d);
    S(TX_VIEW_ONE, "Minimal - one gauge", "Minimal - eine Uhr", d);
    S(TX_VIEW_TWO, "Minimal - two gauges", "Minimal - zwei Uhren", d);
    S(TX_VIEW_FULL, "Full panel", "Volles Panel", d);
    S(TX_SHOW, "Show", "Anzeigen", d);
    S(TX_SH_SPEED, "Speedometer", "Tacho", d);
    S(TX_SH_RPM_RING, "RPM ring", "Drehzahlring", d);
    S(TX_SH_RPM, "Tachometer", "Drehzahlmesser", d);
    S(TX_SH_CRUISE, "Cruise control mark", "Tempomat-Marke", d);
    S(TX_SH_BOOST, "Boost (turbo only)", "Ladedruck (nur mit Turbo)", d);
    S(TX_SH_TURBO, "Turbo", "Turbo", d);
    S(TX_SH_PITCH, "Pitch", "Neigung l\u00e4ngs", d);
    S(TX_SH_ROLL, "Roll", "Neigung quer", d);
    S(TX_SH_GEARSTRIP, "Gear strip", "Gangleiste", d);
    S(TX_SH_LAMPS, "Warning lights", "Kontrollleuchten", d);
    S(TX_SH_VIEWBTN, "'Full panel' button", "Knopf 'Volles Panel'", d);
    S(TX_SH_GEARBOX, "Gearbox", "Getriebe", d);
    S(TX_SH_AIDS, "Driving aids & engine", "Fahrhilfen & Motor", d);
    S(TX_SH_LIGHTS, "Lights", "Licht", d);
    S(TX_SH_TYRES, "Tyres & drive", "Reifen & Antrieb", d);
    S(TX_COLORS, "Colours", "Farben", d);
    S(TX_COL_SPEED, "Speedometer", "Tacho", d);
    S(TX_COL_RPM, "Tachometer", "Drehzahl", d);
    S(TX_COL_RESET, "Reset colours", "Farben zur\u00fccksetzen", d);
    S(TX_UNITS, "Units", "Einheiten", d);
    S(TX_UNITS_GAME, "as in game settings", "wie Spieleinstellung", d);
    S(TX_COL_BG, "Background", "Hintergrund", d);
    S(TX_BG_OPACITY, "Background opacity %", "Deckkraft Hintergrund %", d);
    S(TX_SIZE, "Size", "Gr\u00f6\u00dfe", d);
    S(TX_ARRANGE, "Arrange", "Anordnen", d);
    S(TX_ARRANGE_DONE, "Done", "Fertig", d);
    S(TX_ARRANGE_DONE_TIP, "End arranging (or right click a part)", "Anordnen beenden (oder Rechtsklick auf ein Teil)", d);
    S(TX_SNAP, "Snap to grid", "Am Raster einrasten", d);
    S(TX_ARRANGE_HINT, "Drag the framed parts with the mouse; 'Done' or a right click ends it",
                       "Umrahmte Teile mit der Maus verschieben; 'Fertig' oder Rechtsklick beendet", d);
    S(TX_RESET_LAYOUT, "Reset arrangement", "Anordnung zur\u00fccksetzen", d);
    S(TX_OTHER_DASH, "Other dashboards: Settings > UI > Default truck dashboard",
                     "Andere Dashboards: Einstellungen > UI > Default truck dashboard", d);
    S(TX_KEY, "Key", "Taste", d);
    S(TX_TIP_SETTINGS, "Dashboard settings", "Dashboard-Einstellungen", d);
    S(TX_TIP_FULL, "Show the full panel", "Volles Panel anzeigen", d);
    S(TX_TIP_SMALL, "Back to the small view", "Zur\u00fcck zur kleinen Ansicht", d);
    S(TX_TIP_LIMITER, "Set by the vehicle", "Vom Fahrzeug vorgegeben", d);
    S(TX_TIP_HOLD, "Hold", "Gedr\u00fcckt halten", d);
    S(TX_TIP_ENGINE, "Click to start or stop the engine", "Klick: Motor starten oder abstellen", d);
}

string T(Txt id) { return g_txt[id]; }

// ---- Settings (kept in 'imgui_dashboards.asdata') ----

const int VIEW_ONE = 0;
const int VIEW_TWO = 1;
const int VIEW_FULL = 2;

const int SH_SPEED = 0;
const int SH_RPM = 1;      // RPM gauge (two gauges, full panel) / RPM ring (one gauge)
const int SH_CRUISE = 2;
const int SH_BOOST = 3;    // boost gauge / turbo bar
const int SH_PITCH = 4;
const int SH_ROLL = 5;
const int SH_GEAR = 6;     // gear strip / gearbox card
const int SH_LAMPS = 7;    // warning lights / driving aids & engine card
const int SH_LIGHTS = 8;   // lights card (full panel)
const int SH_TYRES = 9;    // tyres & drive card (full panel)
const int SH_VIEWBTN = 10;
const int SH_COUNT = 11;

int   g_view = VIEW_TWO;
int   g_last_small_view = VIEW_TWO;
int   g_units = 0;         // 0 = game setting, 1 = km/h, 2 = mph
color g_col_bg = COL_DIAL_DEF;   // instrument background
int   g_bg_pct = BG_PCT_DEF;      // instrument background opacity [%]
color g_col_speed = COL_ACCENT;  // scale arc of the speedometer
color g_col_rpm = COL_ACCENT;    // scale arc of the tachometer / power gauge, turbo bar
float g_scale_mul = 1.f;
array<bool> g_show = { true, true, true, true, true, true, true, true, true, false, true };
bool  g_save_pending = false;
LocalStorageClass@ g_storage;

void LoadSettings()
{
    @g_storage = LocalStorageClass("imgui_dashboards");
    if (g_storage.exists("car_view"))        g_view = clamp_i(g_storage.getInt("car_view"), 0, 2);
    if (g_storage.exists("car_small_view"))  g_last_small_view = clamp_i(g_storage.getInt("car_small_view"), 0, 1);
    if (g_storage.exists("car_units"))       g_units = clamp_i(g_storage.getInt("car_units"), 0, 2);
    if (g_storage.exists("car_bg_pct"))         g_bg_pct = clamp_i(g_storage.getInt("car_bg_pct"), 0, 100);
    else if (g_storage.exists("car_transparent") && g_storage.getBool("car_transparent")) g_bg_pct = 0; // older setting
    if (g_storage.exists("car_scale"))       g_scale_mul = clampf(g_storage.getFloat("car_scale"), 0.5f, 2.f);
    LoadColor("car_col_speed", g_col_speed);
    LoadColor("car_col_rpm", g_col_rpm);
    LoadColor("car_col_bg", g_col_bg);
    for (int i = 0; i < SH_COUNT; i++)
    {
        if (g_storage.exists("car_show" + i))
            g_show[i] = g_storage.getBool("car_show" + i);
    }
    if (g_storage.exists("car_snap")) g_snap = g_storage.getBool("car_snap");
    for (int v = 0; v < 3; v++)
    {
        if (g_storage.exists("car_grp" + v + "_x")) { g_grp_x[v] = g_storage.getFloat("car_grp" + v + "_x"); g_grp_y[v] = g_storage.getFloat("car_grp" + v + "_y"); }
        for (int e = 0; e < EL_COUNT; e++)
        {
            const string key = "car_off" + v + "_" + e;
            if (g_storage.exists(key + "_x")) { g_off_x[v * EL_COUNT + e] = g_storage.getFloat(key + "_x"); g_off_y[v * EL_COUNT + e] = g_storage.getFloat(key + "_y"); }
        }
    }
}

void SaveSettings()
{
    g_storage.setInt("car_view", g_view);
    g_storage.setInt("car_small_view", g_last_small_view);
    g_storage.setInt("car_units", g_units);
    g_storage.setInt("car_bg_pct", g_bg_pct);
    g_storage.setFloat("car_scale", g_scale_mul);
    SaveColor("car_col_speed", g_col_speed);
    SaveColor("car_col_rpm", g_col_rpm);
    SaveColor("car_col_bg", g_col_bg);
    for (int i = 0; i < SH_COUNT; i++)
        g_storage.setBool("car_show" + i, g_show[i]);
    g_storage.setBool("car_snap", g_snap);
    for (int v = 0; v < 3; v++)
    {
        g_storage.setFloat("car_grp" + v + "_x", g_grp_x[v]);
        g_storage.setFloat("car_grp" + v + "_y", g_grp_y[v]);
        for (int e = 0; e < EL_COUNT; e++)
        {
            const string key = "car_off" + v + "_" + e;
            if (g_off_x[v * EL_COUNT + e] != 0.f || g_off_y[v * EL_COUNT + e] != 0.f || g_storage.exists(key + "_x"))
            {
                g_storage.setFloat(key + "_x", g_off_x[v * EL_COUNT + e]);
                g_storage.setFloat(key + "_y", g_off_y[v * EL_COUNT + e]);
            }
        }
    }
    g_storage.save();
}

void LoadColor(const string&in key, color&inout col) // not "&out": that would overwrite 'col' with white when the key is missing
{
    if (g_storage.exists(key + "_r"))
        col = color(g_storage.getFloat(key + "_r"), g_storage.getFloat(key + "_g"), g_storage.getFloat(key + "_b"), 1.f);
}

void SaveColor(const string&in key, const color&in col)
{
    g_storage.setFloat(key + "_r", col.r);
    g_storage.setFloat(key + "_g", col.g);
    g_storage.setFloat(key + "_b", col.b);
}

int clamp_i(int v, int lo, int hi) { return (v < lo) ? lo : ((v > hi) ? hi : v); }

/// Instrument background (dials, gear strip, warning lights, cards) with the chosen opacity
color BgCol(float lighten = 0.f)
{
    return color(fmin(1.f, g_col_bg.r + lighten), fmin(1.f, g_col_bg.g + lighten), fmin(1.f, g_col_bg.b + lighten), g_bg_pct / 100.f);
}

color BgBorder()
{
    return color(1.f, 1.f, 1.f, 0.18f * fmin(1.f, g_bg_pct / 50.f));
}

// ---- Input events (buttons act like the keys) ----

array<int> g_pulses;     // events released at the start of the next frame
array<int> g_held;       // events held by buttons in the previous frame
array<int> g_held_now;   // events held by buttons in this frame

bool Contains(const array<int>&in a, int v)
{
    for (uint i = 0; i < a.length(); i++)
    {
        if (a[i] == v)
            return true;
    }
    return false;
}

/// Toggle: event active for exactly one frame
void Pulse(inputEvents ev)
{
    thisActor.setEventSimulatedValue(ev, 1.f);
    g_pulses.insertLast(int(ev));
}

void ReleaseInputs()
{
    for (uint i = 0; i < g_pulses.length(); i++)
        thisActor.resetEventSimulatedValue(inputEvents(g_pulses[i]));
    g_pulses.resize(0);
    for (uint i = 0; i < g_held.length(); i++)
    {
        if (!Contains(g_held_now, g_held[i]))
            inputs.setEventSimulatedValue(inputEvents(g_held[i]), 0.f);
    }
    g_held = g_held_now;
    g_held_now.resize(0);
}

string KeyOf(inputEvents ev)
{
    const string k = inputs.getEventCommandTrimmed(ev);
    return (k != "") ? T(TX_KEY) + ": " + k : "";
}

// ---- Vehicle readings (once per frame) ----

class Readings
{
    float speed_ms;      // wheel speed, like the classic speedometer
    float speed;         // in display units
    float speed_max;     // scale end in display units
    string speed_unit;
    float cruise;        // cruise control target in display units; < 0 = off
    float limit;         // speed limiter in display units; < 0 = off
    bool  imperial;

    bool  has_engine;
    bool  electric;
    bool  running;
    float rpm;
    float rpm_max;       // scale end
    float rpm_red;       // red from here (shift-up RPM)
    float rpm_major;     // labelled step
    float rpm_minor;     // tick step
    float rpm_div;       // label divisor (100 or 1000)
    float power;         // electric: -100..100 %

    bool  turbo;
    float boost;         // in display units (psi or bar)
    float boost_max;
    string boost_unit;

    int   gear;          // -1 = R, 0 = N
    int   num_gears;
    SimGearboxMode mode;
    int   autoselect;    // autoswitch
    int   gear_range;
    int   num_ranges;

    float pitch;         // deg, > 0 = nose up
    float roll;          // deg, > 0 = leaning right
    float slip;          // 0..1
}

Readings r;
float g_speed_max_kmh = 0.f;   // grows when exceeded, per vehicle
float g_boost_max_psi = 25.f;
int   g_actor_id = -1;
float g_blink_time = 0.f;

void Read(float dt)
{
    const bool game_imperial = (console.cVarFind("gfx_speedo_imperial") !is null) && console.cVarFind("gfx_speedo_imperial").getBool();
    r.imperial = (g_units == 0) ? game_imperial : (g_units == 2);
    const float f = r.imperial ? MS_TO_MPH : MS_TO_KMH;
    r.speed_unit = r.imperial ? "mph" : "km/h";
    r.speed_ms = abs(thisActor.getWheelSpeed());
    r.speed = r.speed_ms * f;

    EngineClass@ eng = thisActor.getEngine();
    r.has_engine = (eng !is null);
    if (g_actor_id != thisActor.getInstanceId())
    {
        g_actor_id = thisActor.getInstanceId();
        g_speed_max_kmh = (r.has_engine && eng.getEngineType() == 116) ? 120.f : 160.f; // 't' = truck
        g_boost_max_psi = 25.f;
    }
    while (r.speed_ms * MS_TO_KMH > g_speed_max_kmh * 0.97f && g_speed_max_kmh < 400.f)
        g_speed_max_kmh += 40.f;
    r.speed_max = r.imperial ? Ceil(g_speed_max_kmh / 1.609344f, 20.f) : g_speed_max_kmh;

    r.cruise = thisActor.getCruiseControl() ? thisActor.getCruiseControlSpeed() * f : -1.f;
    r.limit = thisActor.getSpeedLimiterEnabled() ? thisActor.getSpeedLimit() * f : -1.f;

    r.electric = false; r.running = false; r.turbo = false;
    r.rpm = 0.f; r.power = 0.f; r.boost = 0.f; r.gear = 0; r.num_gears = 0;
    r.autoselect = AUTOSWITCH_NEUTRAL; r.gear_range = 0; r.num_ranges = 1;
    if (r.has_engine)
    {
        r.electric = eng.isElectric() || eng.getEngineType() == 101; // 'e'
        r.running = eng.isRunning();
        r.rpm = eng.getRPM();
        const float up = fmax(eng.getShiftUpRPM(), 1000.f);
        r.rpm_max = fmax(2000.f, Ceil(up * 1.2f, 1000.f));
        r.rpm_red = up;
        if (r.rpm_max <= 4000.f) { r.rpm_div = 100.f; r.rpm_major = 500.f; r.rpm_minor = 100.f; }
        else                     { r.rpm_div = 1000.f; r.rpm_major = 1000.f; r.rpm_minor = 250.f; }
        const float max_torque = eng.getEngineTorque();
        r.power = (max_torque > 0.f) ? clampf(eng.getCurEngineTorque() / max_torque * 100.f, -40.f, 100.f) : 0.f;
        r.turbo = eng.hasTurbo();
        if (r.turbo)
        {
            const float psi = fmax(0.f, eng.getTurboPSI());
            while (psi > g_boost_max_psi * 0.95f && g_boost_max_psi < 100.f)
                g_boost_max_psi += 5.f;
            r.boost = r.imperial ? psi : psi * PSI_TO_BAR;
            r.boost_max = r.imperial ? g_boost_max_psi : Ceil(g_boost_max_psi * PSI_TO_BAR, 0.5f);
            r.boost_unit = r.imperial ? "psi" : "bar";
        }
        r.gear = eng.getGear();
        r.num_gears = eng.getNumGears();
        r.mode = eng.getAutoMode();
        r.autoselect = int(eng.getAutoShift());
        r.gear_range = eng.getGearRange();
        r.num_ranges = eng.getNumGearsRanges();
        const float ground = abs(thisActor.getSpeed());
        r.slip = (r.speed_ms > 1.f) ? clampf((r.speed_ms - ground) / r.speed_ms, 0.f, 1.f) : 0.f;
    }

    // Tilt from the camera reference nodes (same as the boat dashboard)
    const vector3 dir = thisActor.getDirection();
    r.pitch = asin(clampf(dir.y, -1.f, 1.f)) / DEG2RAD;
    r.roll = 0.f;
    const int num_nodes = thisActor.getNodeCount();
    if (thisActor.getCameraCount() > 0)
    {
        const int pos_node = thisActor.getCameraPosNode(0);
        const int roll_node = thisActor.getCameraRollNode(0);
        if (pos_node >= 0 && roll_node >= 0 && pos_node < num_nodes && roll_node < num_nodes && pos_node != roll_node)
        {
            vector3 side = thisActor.getNodePosition(pos_node) - thisActor.getNodePosition(roll_node);
            side.normalise();
            vector3 right = dir.crossProduct(vector3(0.f, 1.f, 0.f));
            right.normalise();
            if (side.dotProduct(right) < 0.f)
                side = side * -1.f; // make it point to the right
            r.roll = -asin(clampf(side.y, -1.f, 1.f)) / DEG2RAD;
        }
    }
    g_blink_time += dt;
}

float Ceil(float v, float step) { return ceil(v / step - 0.001f) * step; }

// ---- Drawing in design units ----

class Ctx
{
    ImDrawList@ dl;
    vector2 o;   // window origin (screen px)
    float k;     // px per design unit

    vector2 P(float x, float y) { return vector2(o.x + x * k, o.y + y * k); }
    vector2 Pol(float cx, float cy, float r, float deg) // deg clockwise from top
    {
        const float t = deg * DEG2RAD;
        return P(cx + r * sin(t), cy - r * cos(t));
    }
    vector2 Size(const string&in s, float size)
    {
        const vector2 ts = ImGui::CalcTextSize(s);
        const float f = size * k / ImGui::GetTextLineHeight();
        return vector2(ts.x * f, ts.y * f);
    }
    void TextAt(vector2 p, const string&in s, float size, const color&in col) // centered at a screen position
    {
        const vector2 ts = Size(s, size);
        dl.AddText(vector2(p.x - ts.x * 0.5f, p.y - ts.y * 0.5f), col, s, size * k);
    }
    void Text(float cx, float cy, const string&in s, float size, const color&in col) // centered
    {
        TextAt(P(cx, cy), s, size, col);
    }
    void TextL(float x, float cy, const string&in s, float size, const color&in col) // left aligned, vertically centered
    {
        const vector2 ts = Size(s, size);
        const vector2 p = P(x, cy);
        dl.AddText(vector2(p.x, p.y - ts.y * 0.5f), col, s, size * k);
    }
    void TextR(float x, float cy, const string&in s, float size, const color&in col) // right aligned
    {
        const vector2 ts = Size(s, size);
        const vector2 p = P(x, cy);
        dl.AddText(vector2(p.x - ts.x, p.y - ts.y * 0.5f), col, s, size * k);
    }
    void Line(float x1, float y1, float x2, float y2, const color&in col, float w)
    {
        dl.AddLine(P(x1, y1), P(x2, y2), col, w * k);
    }
    void Rect(float x, float y, float w, float h, const color&in fill, const color&in border, float rounding)
    {
        dl.AddRectFilled(P(x, y), P(x + w, y + h), fill, rounding * k);
        if (border.a > 0.f)
            dl.AddRect(P(x, y), P(x + w, y + h), border, rounding * k, 15, 1.f);
    }
    void Card(float x, float y, float w, float h, const string&in title)
    {
        Rect(x, y, w, h, BgCol(0.035f), COL_CARD_BR, 6.f);
        TextL(x + 9.f, y + 12.f, title, 9.f, COL_MUTED);
    }
    /// Arc stroke, angles in degrees clockwise from top
    void Arc(float cx, float cy, float r, float a0, float a1, const color&in col, float w)
    {
        if (a1 <= a0)
            return;
        dl.PathClear();
        dl.PathArcTo(P(cx, cy), r * k, (a0 - 90.f) * DEG2RAD, (a1 - 90.f) * DEG2RAD, int(fmax(4.f, (a1 - a0) / 6.f)));
        dl.PathStroke(col, false, w * k);
    }
    /// Filled convex polygon from design-unit points, rotated by 'deg' around (cx, cy)
    void Poly(const array<float>&in pts, float cx, float cy, float deg, const color&in col)
    {
        const float s = sin(deg * DEG2RAD), co = cos(deg * DEG2RAD);
        dl.PathClear();
        for (uint i = 0; i + 1 < pts.length(); i += 2)
        {
            const float x = pts[i] - cx, y = pts[i + 1] - cy;
            dl.PathLineTo(P(cx + x * co - y * s, cy + x * s + y * co));
        }
        dl.PathFillConvex(col);
    }
}

Ctx c;
int g_idc = 0;

// Clickable area; 'active' = mouse button held on it
bool Area(float x, float y, float w, float h, bool &out active)
{
    ImGui::SetCursorScreenPos(c.P(x, y));
    ImGui::PushID(g_idc++);
    ImGui::InvisibleButton("a", vector2(fmax(1.f, w * c.k), fmax(1.f, h * c.k)));
    ImGui::PopID();
    active = ImGui::IsItemActive();
    return ImGui::IsItemClicked(0);
}

const int ST_OFF = 0;
const int ST_ON = 1;    // blue
const int ST_GREEN = 2;
const int ST_WARN = 3;  // amber outline
const int ST_BG = 4;    // like ST_OFF, on the instrument background

bool Button(float x, float y, float w, float h, const string&in label, int state, const string&in tooltip, float size = 9.f)
{
    bool active;
    const bool clicked = Area(x, y, w, h, active);
    const bool hovered = ImGui::IsItemHovered();
    color fill = hovered ? COL_BTN_HOV : COL_BTN;
    color border = COL_CARD_BR;
    color text = COL_TEXT;
    if (state == ST_ON)    { fill = COL_ACCENT; border = COL_ACCENT; text = COL_INK; }
    if (state == ST_GREEN) { fill = COL_GREEN_BG; border = COL_GREEN; }
    if (state == ST_WARN)  { fill = COL_WARN_BG; border = COL_WARN; text = COL_WARN; }
    if (state == ST_BG)    { fill = hovered ? BgCol(0.1f) : BgCol(0.05f); border = BgBorder(); }
    c.Rect(x, y, w, h, fill, border, 4.f);
    c.Text(x + w * 0.5f, y + h * 0.5f, label, size, text);
    if (hovered && tooltip != "")
        ImGui::SetTooltip(Esc(tooltip));
    return clicked;
}

void GearIcon(float cx, float cy, bool hovered)
{
    const color col = hovered ? COL_TEXT : COL_ACCENT;
    c.dl.AddCircle(c.P(cx, cy), 3.4f * c.k, col, 12, 1.4f * c.k);
    for (int i = 0; i < 8; i++)
        c.dl.AddLine(c.Pol(cx, cy, 4.4f, i * 45.f), c.Pol(cx, cy, 6.2f, i * 45.f), col, 1.6f * c.k);
}

// ---- Gauges ----

/// Round gauge, 270 degrees; returns nothing. Angles: -135 (min) .. +135 (max)
void Gauge(float cx, float cy, float R, bool dial, float vmin, float vmax, float major, float minor, float label_div,
           float val, float red_from, float zero, const color&in fill, float label_size)
{
    const float k = c.k;
    if (dial)
    {
        c.dl.AddCircleFilled(c.P(cx, cy), R * k, BgCol(), 48);
        c.dl.AddCircle(c.P(cx, cy), R * k, BgBorder(), 48, 1.f);
    }
    const float rr = R * 0.94f;            // scale arc
    const float aw = fmax(2.f, R * 0.04f); // arc width
    c.Arc(cx, cy, rr, -135.f, 135.f, COL_TRACK, aw);
    if (red_from < vmax)
        c.Arc(cx, cy, rr, GA(red_from, vmin, vmax), 135.f, COL_RED, aw);
    const float lo = fmin(zero, val), hi = fmax(zero, val);
    if (hi > lo)
        c.Arc(cx, cy, rr, GA(lo, vmin, vmax), GA(hi, vmin, vmax), (val >= zero) ? fill : COL_GREEN, aw);
    const int n = round_i((vmax - vmin) / minor);
    for (int i = 0; i <= n; i++)
    {
        const float v = vmin + i * minor;
        const float a = GA(v, vmin, vmax);
        const float q = (v - vmin) / major;
        const bool mj = abs(q - round_i(q)) < 0.01f;
        const color col = (v >= red_from - 0.001f) ? COL_RED : ((v < zero - 0.001f) ? COL_GREEN : COL_TICK);
        c.dl.AddLine(c.Pol(cx, cy, R * 0.90f, a), c.Pol(cx, cy, R * (mj ? 0.80f : 0.85f), a), col, (mj ? 2.2f : 1.2f) * R / 100.f * k);
        if (mj && label_size > 0.f)
            c.TextAt(c.Pol(cx, cy, R * 0.67f, a), formatFloat(v / label_div, "", 0, (label_div < 1.f) ? 1 : 0), label_size, col);
    }
}

float GA(float v, float vmin, float vmax)
{
    return -135.f + clampf((v - vmin) / (vmax - vmin), 0.f, 1.f) * 270.f;
}

void Needle(float cx, float cy, float R, float a, bool short_pointer)
{
    const float k = c.k;
    if (short_pointer) // pointer at the scale only (the center shows numbers)
    {
        c.dl.AddLine(c.Pol(cx, cy, R * 0.66f, a), c.Pol(cx, cy, R * 0.92f, a), COL_TEXT, 4.f * R / 100.f * k);
        return;
    }
    c.dl.AddLine(c.P(cx, cy), c.Pol(cx, cy, R * 0.86f, a), COL_TEXT, 3.5f * R / 100.f * k);
    c.dl.AddCircleFilled(c.P(cx, cy), 7.f * R / 100.f * k, COL_TEXT, 16);
}

void CruiseMark(float cx, float cy, float R, float a)
{
    c.dl.AddCircleFilled(c.Pol(cx, cy, R * 0.94f, a), 5.f * R / 100.f * c.k, COL_WARN, 12);
}

string GearText()
{
    if (!r.has_engine)
        return "";
    if (r.mode == AUTO)
    {
        switch (r.autoselect)
        {
        case AUTOSWITCH_REAR: return "R";
        case AUTOSWITCH_NEUTRAL: return "N";
        case AUTOSWITCH_TWO: return "2";
        case AUTOSWITCH_ONE: return "1";
        default: return (r.gear > 0) ? "D" + r.gear : "D";
        }
    }
    if (r.gear < 0) return "R";
    if (r.gear == 0) return "N";
    return formatInt(r.gear);
}

void SpeedGauge(float cx, float cy, float R, bool dial, bool big_number, float num_size = 0.40f)
{
    const float step = (r.speed_max > 200.f) ? 40.f : 20.f;
    Gauge(cx, cy, R, dial, 0.f, r.speed_max, step, step * 0.5f, 1.f, r.speed, 1e9f, 0.f, g_col_speed, R * 0.15f);
    if (g_show[SH_CRUISE] && r.cruise >= 0.f)
        CruiseMark(cx, cy, R, GA(r.cruise, 0.f, r.speed_max));
    if (big_number)
    {
        c.Text(cx, cy + R * 0.46f, formatInt(round_i(r.speed)), R * num_size, COL_TEXT);
        c.Text(cx, cy + R * 0.70f, r.speed_unit, R * 0.13f, COL_MUTED);
    }
    Needle(cx, cy, R, GA(r.speed, 0.f, r.speed_max), false);
}

void RpmGauge(float cx, float cy, float R, bool dial)
{
    if (r.electric)
    {
        Gauge(cx, cy, R, dial, -40.f, 100.f, 20.f, 10.f, 1.f, r.power, 1e9f, 0.f, g_col_rpm, R * 0.15f);
        c.Text(cx, cy + R * 0.34f, formatInt(round_i(r.power)) + " %", R * 0.22f, COL_TEXT);
        c.Text(cx, cy + R * 0.52f, T(TX_POWER), R * 0.12f, COL_MUTED);
        c.Text(cx, cy + R * 0.84f, T(TX_REGEN), R * 0.11f, COL_GREEN);
        Needle(cx, cy, R, GA(r.power, -40.f, 100.f), false);
        return;
    }
    Gauge(cx, cy, R, dial, 0.f, r.rpm_max, r.rpm_major, r.rpm_minor, r.rpm_div, r.rpm, r.rpm_red, 0.f, g_col_rpm, R * 0.15f);
    c.Text(cx, cy + R * 0.40f, T((r.rpm_div > 100.f) ? TX_RPM_X1000 : TX_RPM_X100), R * 0.13f, COL_MUTED);
    c.Text(cx, cy + R * 0.70f, GearText(), R * 0.28f, GearColor(GearText().substr(0, 1)));
    Needle(cx, cy, R, GA(r.rpm, 0.f, r.rpm_max), false);
}

void BoostGauge(float cx, float cy, float R)
{
    const float step = (r.boost_max > 3.f) ? 10.f : 1.f;
    Gauge(cx, cy, R, true, 0.f, r.boost_max, step, step * 0.5f, 1.f, r.boost, 1e9f, 0.f, COL_WARN, R * 0.26f);
    c.Text(cx, cy + R * 0.50f, formatFloat(r.boost, "", 0, (r.boost_max > 3.f) ? 0 : 1), R * 0.34f, COL_TEXT);
    c.Text(cx, cy + R * 0.78f, r.boost_unit, R * 0.20f, COL_MUTED);
    Needle(cx, cy, R, GA(r.boost, 0.f, r.boost_max), false);
}

/// Tilt gauge: scale -45..45 at the top, vehicle silhouette rotated by the angle
void TiltGauge(float cx, float cy, float R, float deg, bool side_view)
{
    const float k = c.k;
    c.dl.AddCircleFilled(c.P(cx, cy), R * k, BgCol(), 32);
    c.dl.AddCircle(c.P(cx, cy), R * k, BgBorder(), 32, 1.f);
    for (int d = -45; d <= 45; d += 15)
    {
        const color col = (abs(d) >= 30) ? COL_RED : COL_TICK;
        c.dl.AddLine(c.Pol(cx, cy, R * 0.94f, d * 1.5f), c.Pol(cx, cy, R * 0.80f, d * 1.5f), col, 2.f * k);
    }
    const float shown = clampf(deg, -60.f, 60.f);
    c.dl.AddLine(c.Pol(cx, cy, R * 0.80f, shown * 1.5f), c.Pol(cx, cy, R * 0.97f, shown * 1.5f), COL_WARN, 3.f * k);
    const float u = R / 100.f; // silhouette units
    const float py = cy + 10.f * u;
    if (side_view)
    {
        // nose to the right; positive pitch = nose up = counter-clockwise on screen
        const float a = -shown;
        array<float> body = { cx - 48 * u, py + 2 * u, cx + 48 * u, py + 2 * u, cx + 48 * u, py - 6 * u, cx - 48 * u, py - 6 * u };
        array<float> cab = { cx + 8 * u, py - 6 * u, cx + 30 * u, py - 6 * u, cx + 42 * u, py - 6 * u, cx + 30 * u, py - 26 * u, cx + 8 * u, py - 26 * u };
        c.Poly(body, cx, py, a, COL_TEXT);
        c.Poly(cab, cx, py, a, COL_TEXT);
        WheelAt(cx - 32 * u, py + 8 * u, 9 * u, cx, py, a);
        WheelAt(cx + 32 * u, py + 8 * u, 9 * u, cx, py, a);
    }
    else
    {
        // seen from behind; positive roll = leaning right = clockwise on screen
        const float a = shown;
        array<float> body = { cx - 28 * u, py + 6 * u, cx + 28 * u, py + 6 * u, cx + 28 * u, py - 24 * u, cx - 28 * u, py - 24 * u };
        array<float> wl = { cx - 36 * u, py + 18 * u, cx - 22 * u, py + 18 * u, cx - 22 * u, py + 2 * u, cx - 36 * u, py + 2 * u };
        array<float> wr = { cx + 22 * u, py + 18 * u, cx + 36 * u, py + 18 * u, cx + 36 * u, py + 2 * u, cx + 22 * u, py + 2 * u };
        array<float> win = { cx - 20 * u, py - 8 * u, cx + 20 * u, py - 8 * u, cx + 20 * u, py - 20 * u, cx - 20 * u, py - 20 * u };
        c.Poly(body, cx, py, a, COL_TEXT);
        c.Poly(wl, cx, py, a, COL_TEXT);
        c.Poly(wr, cx, py, a, COL_TEXT);
        c.Poly(win, cx, py, a, COL_INK);
    }
    c.Text(cx, cy + R * 0.62f, formatFloat(deg, "+", 0, 0) + "\u00b0", R * 0.30f, COL_TEXT);
    c.Text(cx, cy + R * 0.86f, T(side_view ? TX_PITCH : TX_ROLL), R * 0.17f, COL_MUTED);
}

void WheelAt(float x, float y, float rad, float cx, float cy, float deg)
{
    const float s = sin(deg * DEG2RAD), co = cos(deg * DEG2RAD);
    const float dx = x - cx, dy = y - cy;
    c.dl.AddCircleFilled(c.P(cx + dx * co - dy * s, cy + dx * s + dy * co), rad * c.k, COL_TEXT, 12);
}

// ---- Small parts: gear strip, warning lights, buttons ----

const float STRIP_W = 33.f;      // gear strip width (design units)
const float LAMPS_H = 33.f;      // warning light row height
const float LS = 1.5f;           // size of the gear strip and warning lights

/// Colour of a gear: reverse red, neutral white, forward gears green
color GearColor(const string&in g)
{
    if (g == "R") return COL_RED;
    if (g == "N") return COL_TEXT;
    return COL_GREEN;
}

array<string>@ GearStripItems()
{
    array<string> items;
    if (r.mode == AUTO)
    {
        items = { "R", "N", "D" };
        if (!r.electric) { items.insertLast("2"); items.insertLast("1"); }
    }
    else
    {
        items.insertLast("R"); items.insertLast("N");
        const int first = clamp_i(r.gear - 2, 1, (r.num_gears > 5) ? r.num_gears - 4 : 1);
        for (int g = first; g <= r.num_gears && g < first + 5; g++)
            items.insertLast(formatInt(g));
    }
    return items;
}

float GearStripHeight()
{
    return GearStripItems().length() * 13.f * LS + 10.f * LS;
}

void GearStrip(float x, float y_bottom)
{
    array<string>@ items = GearStripItems();
    string cur = GearText();
    if (r.mode == AUTO && cur.length() > 1)
        cur = "D";
    const float step = 13.f * LS;
    const float h = items.length() * step + 10.f * LS;
    const float y = y_bottom - h;
    c.Rect(x, y, STRIP_W, h, BgCol(), BgBorder(), 6.f * LS);
    for (uint i = 0; i < items.length(); i++)
    {
        const float iy = y + 11.f * LS + i * step;
        const color gc = GearColor(items[i]);
        if (items[i] == cur)
        {
            c.Rect(x + 3.f * LS, iy - 6.f * LS, STRIP_W - 6.f * LS, 12.f * LS, gc, Col(0, 0, 0, 0), 3.f * LS);
            c.Text(x + STRIP_W * 0.5f, iy, items[i], 9.f * LS, COL_INK);
        }
        else
        {
            c.Text(x + STRIP_W * 0.5f, iy, items[i], 9.f * LS, color(gc.r, gc.g, gc.b, 0.35f));
        }
    }
}

/// Row of warning lights; returns the width
const int LAMP_LEFT = 0, LAMP_RIGHT = 1, LAMP_LOW = 2, LAMP_HIGH = 3, LAMP_FOG = 4, LAMP_PBRAKE = 5, LAMP_TEXT = 6, LAMP_ENGINE = 7;

float Lamps(float x_right, float y, bool draw)
{
    const bool blink_on = (int(g_blink_time * 2.5f) % 2) == 0;
    const int blink = thisActor.getBlinkType();
    array<int> kinds;
    array<string> labels;
    array<color> cols;
    const color OFF = COL_DARK;
    kinds.insertLast(LAMP_LEFT);  labels.insertLast(""); cols.insertLast((blink_on && (blink == BLINK_LEFT || blink == BLINK_WARN)) ? COL_GREEN : OFF);
    kinds.insertLast(LAMP_RIGHT); labels.insertLast(""); cols.insertLast((blink_on && (blink == BLINK_RIGHT || blink == BLINK_WARN)) ? COL_GREEN : OFF);
    kinds.insertLast(LAMP_LOW);   labels.insertLast(""); cols.insertLast(thisActor.getHeadlightsVisible() ? COL_GREEN : OFF);
    if (thisActor.countFlaresByType(FLARE_TYPE_HIGH_BEAM) > 0)
    {
        kinds.insertLast(LAMP_HIGH); labels.insertLast(""); cols.insertLast(thisActor.getHighBeamsVisible() ? COL_ACCENT : OFF);
    }
    if (thisActor.countFlaresByType(FLARE_TYPE_FOG_LIGHT) > 0)
    {
        kinds.insertLast(LAMP_FOG); labels.insertLast(""); cols.insertLast(thisActor.getFogLightsVisible() ? COL_GREEN : OFF);
    }
    kinds.insertLast(LAMP_PBRAKE); labels.insertLast("P"); cols.insertLast(thisActor.getParkingBrake() ? COL_RED : OFF);
    kinds.insertLast(LAMP_TEXT); labels.insertLast("ABS"); cols.insertLast(thisActor.getAntiLockBrake() ? COL_WARN : OFF);
    kinds.insertLast(LAMP_TEXT); labels.insertLast("TC"); cols.insertLast(thisActor.getTractionControl() ? COL_WARN : OFF);
    kinds.insertLast(LAMP_TEXT); labels.insertLast("CC"); cols.insertLast(thisActor.getCruiseControl() ? COL_ACCENT : OFF);
    if (r.has_engine && !r.running)
    {
        kinds.insertLast(LAMP_ENGINE); labels.insertLast("!"); cols.insertLast(COL_RED);
    }
    float w = 12.f * LS;
    for (uint i = 0; i < kinds.length(); i++)
        w += ((kinds[i] == LAMP_TEXT) ? 22.f : 14.f) * LS + 5.f * LS;
    if (!draw)
        return w;
    const float x0 = x_right - w;
    c.Rect(x0, y, w, LAMPS_H, BgCol(), BgBorder(), 6.f * LS);
    float x = x0 + 8.f * LS;
    const float cy = y + LAMPS_H * 0.5f;
    for (uint i = 0; i < kinds.length(); i++)
    {
        const float lw = ((kinds[i] == LAMP_TEXT) ? 22.f : 14.f) * LS;
        const color col = cols[i];
        const float mx = x + lw * 0.5f;
        switch (kinds[i])
        {
        case LAMP_LEFT:
            Arrow(mx, cy, -1.f, col, LS);
            break;
        case LAMP_RIGHT:
            Arrow(mx, cy, 1.f, col, LS);
            break;
        case LAMP_LOW:
        case LAMP_HIGH:
        case LAMP_FOG:
        {
            // lamp body (half circle) with beams; low beam points down, fog light has a vertical cut
            c.dl.PathClear();
            c.dl.PathArcTo(c.P(mx - 1.f * LS, cy), 5.f * LS * c.k, PI_F * 0.5f, PI_F * 1.5f, 8);
            c.dl.PathFillConvex(col);
            const float d = (kinds[i] == LAMP_LOW) ? 2.f * LS : 0.f;
            for (int b = -1; b <= 1; b++)
                c.Line(mx + 1.f * LS, cy + b * 3.5f * LS, mx + 7.f * LS, cy + b * 3.5f * LS + d, col, 1.4f * LS);
            if (kinds[i] == LAMP_FOG)
                c.Line(mx + 4.f * LS, cy - 6.f * LS, mx + 4.f * LS, cy + 6.f * LS, col, 1.f * LS);
            break;
        }
        case LAMP_PBRAKE:
        case LAMP_ENGINE:
            c.dl.AddCircle(c.P(mx, cy), 6.f * LS * c.k, col, 16, 1.4f * LS * c.k);
            c.Text(mx, cy, labels[i], 8.f * LS, col);
            break;
        default:
            c.dl.AddRect(c.P(x, cy - 6.f * LS), c.P(x + lw, cy + 6.f * LS), col, 2.f * LS * c.k, 15, 1.4f);
            c.Text(mx, cy, labels[i], 7.5f * LS, col);
            break;
        }
        x += lw + 5.f * LS;
    }
    return w;
}

void Arrow(float cx, float cy, float dir, const color&in col, float sc = 1.f)
{
    c.dl.AddTriangleFilled(c.P(cx + dir * 6.f * sc, cy), c.P(cx, cy - 5.5f * sc * dir), c.P(cx, cy + 5.5f * sc * dir), col);
    c.dl.AddRectFilled(c.P(fmin(cx, cx - dir * 5.f * sc), cy - 2.2f * sc), c.P(fmax(cx, cx - dir * 5.f * sc), cy + 2.2f * sc), col);
}

bool g_open_settings = false;

const float TOPBTN_H = 24.f;

float FullButtonWidth()
{
    return g_show[SH_VIEWBTN] ? c.Size(T(TX_FULL_PANEL), 9.f).x / c.k + 18.f + 6.f : 0.f;
}

/// 'Full panel' / settings buttons; right aligned at x_right
void TopButtons(float x_right, float y)
{
    bool active;
    if (Area(x_right - 24.f, y, 24.f, 24.f, active))
        g_open_settings = true;
    const bool hov = ImGui::IsItemHovered();
    c.Rect(x_right - 24.f, y, 24.f, 24.f, BgCol(), hov ? COL_TEXT : COL_ACCENT, 6.f);
    GearIcon(x_right - 12.f, y + 12.f, hov);
    if (hov)
        ImGui::SetTooltip(Esc(T(TX_TIP_SETTINGS)));
    if (g_show[SH_VIEWBTN])
    {
        const string label = T(TX_FULL_PANEL);
        const float w = c.Size(label, 9.f).x / c.k + 18.f;
        if (Button(x_right - 30.f - w, y, w, 24.f, label, ST_BG, T(TX_TIP_FULL)))
        {
            g_last_small_view = g_view;
            g_view = VIEW_FULL;
            SaveSettings();
        }
    }
}

// ---- Layout: every part is a separate element that can be moved ('Arrange' in the settings) ----

const int EL_ONE = 0;        // combined gauge (one-gauge view)
const int EL_BOOST = 1;
const int EL_PITCH = 2;
const int EL_ROLL = 3;
const int EL_RPM = 4;
const int EL_GEAR = 5;       // gear strip
const int EL_SPEED = 6;
const int EL_LAMPS = 7;
const int EL_BUTTONS = 8;    // 'Full panel' + settings
const int EL_HEADER = 9;     // full panel: vehicle name, back + settings
const int EL_C_SPEED = 10;   // full panel cards
const int EL_C_RPM = 11;
const int EL_C_GEAR = 12;
const int EL_C_AIDS = 13;
const int EL_C_LIGHTS = 14;
const int EL_C_TYRES = 15;
const int EL_COUNT = 16;
const float GRID = 10.f;     // design units

class Elem
{
    int id;
    float x, y, w, h;        // default place in design units (concept layout)
}

array<Elem@> g_elems;
array<float> g_off_x(3 * EL_COUNT);   // user offset per view and element [design units]
array<float> g_off_y(3 * EL_COUNT);
array<float> g_grp_x(3);              // whole dashboard moved per view
array<float> g_grp_y(3);
bool  g_arrange = false;
bool  g_snap = true;
int   g_drag_el = -1;
float g_drag_x = 0.f, g_drag_y = 0.f; // unsnapped position while dragging

void AddEl(int id, float x, float y, float w, float h)
{
    Elem e;
    e.id = id; e.x = x; e.y = y; e.w = w; e.h = h;
    g_elems.insertLast(e);
}

/// Size of the current view in design units (concept layout)
vector2 ViewSize()
{
    if (g_view == VIEW_ONE)
        return vector2(fmax(16.f + 2.f * 108.f + STRIP_W + 8.f, Lamps(0.f, 0.f, false) + 16.f), 300.f + LAMPS_H - 22.f);
    if (g_view == VIEW_TWO)
    {
        float w = 8.f;
        if (ShowSmallColumn()) w += 84.f + 8.f;
        if (g_show[SH_RPM] && r.has_engine) w += 212.f + 8.f;
        if (g_show[SH_GEAR] && r.has_engine) w += STRIP_W + 8.f;
        w += fmax(240.f, Lamps(0.f, 0.f, false)) + 8.f;
        return vector2(w, 320.f + LAMPS_H - 22.f);
    }
    return vector2(FullWidth(), 236.f);
}

bool ShowSmallColumn()
{
    return (g_show[SH_BOOST] && r.turbo && !r.electric) || g_show[SH_PITCH] || g_show[SH_ROLL];
}

/// Default (concept) places of the visible elements of the current view
void BuildLayout(float W, float H)
{
    g_elems.resize(0);
    const float lw = Lamps(0.f, 0.f, false);
    if (g_view == VIEW_ONE)
    {
        const float R = 108.f;
        const float cx = W - 8.f - R, cy = H - 8.f - (g_show[SH_LAMPS] ? LAMPS_H + 8.f : 0.f) - R;
        AddEl(EL_ONE, cx - R, cy - R, 2.f * R, 2.f * R);
        if (g_show[SH_GEAR] && r.has_engine)
        {
            const float gh = GearStripHeight();
            AddEl(EL_GEAR, cx - R - STRIP_W - 8.f, cy + R - gh, STRIP_W, gh);
        }
        if (g_show[SH_LAMPS])
            AddEl(EL_LAMPS, W - 8.f - lw, H - 8.f - LAMPS_H, lw, LAMPS_H);
        const float bw = 24.f + FullButtonWidth();
        AddEl(EL_BUTTONS, cx + R - bw, cy - R - 30.f, bw, TOPBTN_H);
    }
    else if (g_view == VIEW_TWO)
    {
        const float bottom = H - 8.f - (g_show[SH_LAMPS] ? LAMPS_H + 8.f : 0.f);
        float x = W - 8.f;
        const float RS = 120.f;
        const float scx = x - fmax(RS, lw * 0.5f);
        if (g_show[SH_SPEED])
            AddEl(EL_SPEED, scx - RS, bottom - 2.f * RS, 2.f * RS, 2.f * RS);
        if (g_show[SH_LAMPS])
            AddEl(EL_LAMPS, x - lw, H - 8.f - LAMPS_H, lw, LAMPS_H);
        const float bw = 24.f + FullButtonWidth();
        AddEl(EL_BUTTONS, x - bw, bottom - 2.f * RS - 30.f, bw, TOPBTN_H);
        x = scx - RS - 8.f;
        if (g_show[SH_GEAR] && r.has_engine)
        {
            const float gh = GearStripHeight();
            AddEl(EL_GEAR, x - STRIP_W, bottom - gh, STRIP_W, gh);
            x -= STRIP_W + 8.f;
        }
        if (g_show[SH_RPM] && r.has_engine)
        {
            AddEl(EL_RPM, x - 212.f, bottom - 212.f, 212.f, 212.f);
            x -= 220.f;
        }
        float y = bottom;
        if (g_show[SH_ROLL])  { AddEl(EL_ROLL, x - 84.f, y - 84.f, 84.f, 84.f); y -= 90.f; }
        if (g_show[SH_PITCH]) { AddEl(EL_PITCH, x - 84.f, y - 84.f, 84.f, 84.f); y -= 90.f; }
        if (g_show[SH_BOOST] && r.turbo && !r.electric) AddEl(EL_BOOST, x - 84.f, y - 84.f, 84.f, 84.f);
    }
    else
    {
        AddEl(EL_HEADER, 8.f, 0.f, HeaderWidth(), 24.f);
        float x = 8.f;
        if (g_show[SH_SPEED]) { AddEl(EL_C_SPEED, x, CARD_Y, 190.f, CARD_H); x += 198.f; }
        if (g_show[SH_RPM] && r.has_engine) { AddEl(EL_C_RPM, x, CARD_Y, 190.f, CARD_H); x += 198.f; }
        if (g_show[SH_GEAR] && r.has_engine) { AddEl(EL_C_GEAR, x, CARD_Y, 170.f, CARD_H); x += 178.f; }
        if (g_show[SH_LAMPS]) { AddEl(EL_C_AIDS, x, CARD_Y, 236.f, CARD_H); x += 244.f; }
        if (g_show[SH_LIGHTS]) { AddEl(EL_C_LIGHTS, x, CARD_Y, 200.f, CARD_H); x += 208.f; }
        if (g_show[SH_TYRES]) { AddEl(EL_C_TYRES, x, CARD_Y, 190.f, CARD_H); x += 198.f; }
    }
}

/// Draws one element in layout coordinates (c.o is set so that the default place lands in its window)
void DrawElement(const Elem@ e)
{
    const float x = e.x, y = e.y, w = e.w, h = e.h;
    switch (e.id)
    {
    case EL_ONE: DrawOneGauge(x + w * 0.5f, y + h * 0.5f, w * 0.5f); break;
    case EL_BOOST: BoostGauge(x + 42.f, y + 42.f, 42.f); break;
    case EL_PITCH: TiltGauge(x + 42.f, y + 42.f, 42.f, r.pitch, true); break;
    case EL_ROLL: TiltGauge(x + 42.f, y + 42.f, 42.f, r.roll, false); break;
    case EL_RPM: RpmGauge(x + 106.f, y + 106.f, 106.f, true); break;
    case EL_GEAR: GearStrip(x, y + h); break;
    case EL_SPEED: SpeedGauge(x + w * 0.5f, y + h * 0.5f, w * 0.5f, true, true); break;
    case EL_LAMPS: Lamps(x + w, y, true); break;
    case EL_BUTTONS: TopButtons(x + w, y); break;
    case EL_HEADER: DrawHeader(x, w); break;
    case EL_C_SPEED: CardSpeed(x, w); break;
    case EL_C_RPM: CardRpm(x, w); break;
    case EL_C_GEAR: CardGearbox(x, w); break;
    case EL_C_AIDS: CardAids(x, w); break;
    case EL_C_LIGHTS: CardLights(x, w); break;
    case EL_C_TYRES: CardTyres(x, w); break;
    }
}

void DrawOneGauge(float cx, float cy, float R)
{
    const float k = c.k;
    // speed scale outside
    const float step = (r.speed_max > 200.f) ? 40.f : 20.f;
    Gauge(cx, cy, R, true, 0.f, r.speed_max, step, step * 0.5f, 1.f, r.speed, 1e9f, 0.f, g_col_speed, 0.f);
    for (float v = 0.f; v <= r.speed_max + 0.1f; v += step * 2.f)
        c.TextAt(c.Pol(cx, cy, R * 0.70f, GA(v, 0.f, r.speed_max)), formatInt(round_i(v)), 14.f, COL_TICK);
    if (g_show[SH_CRUISE] && r.cruise >= 0.f)
        CruiseMark(cx, cy, R, GA(r.cruise, 0.f, r.speed_max));
    // RPM ring inside
    if (g_show[SH_RPM] && r.has_engine)
    {
        const float rr = 52.f;
        if (r.electric)
        {
            c.Arc(cx, cy, rr, -135.f, 135.f, COL_TRACK, 4.f);
            const float a0 = GA(0.f, -40.f, 100.f), a1 = GA(r.power, -40.f, 100.f);
            if (a1 > a0) c.Arc(cx, cy, rr, a0, a1, g_col_rpm, 4.f);
            else         c.Arc(cx, cy, rr, a1, a0, COL_GREEN, 4.f);
        }
        else
        {
            c.Arc(cx, cy, rr, -135.f, 135.f, COL_TRACK, 4.f);
            c.Arc(cx, cy, rr, GA(r.rpm_red, 0.f, r.rpm_max), 135.f, COL_RED, 4.f);
            c.Arc(cx, cy, rr, -135.f, GA(r.rpm, 0.f, r.rpm_max), g_col_rpm, 4.f);
            for (float v = 0.f; v <= r.rpm_max + 1.f; v += 1000.f)
            {
                const float a = GA(v, 0.f, r.rpm_max);
                c.dl.AddLine(c.Pol(cx, cy, rr + 5.f, a), c.Pol(cx, cy, rr + 9.f, a), (v >= r.rpm_red) ? COL_RED : COL_MUTED, 1.5f * k);
            }
        }
    }
    c.Text(cx, cy + 2.f, formatInt(round_i(r.speed)), 40.f, COL_TEXT);
    c.Text(cx, cy + 24.f, r.speed_unit, 11.f, COL_MUTED);
    if (g_show[SH_RPM] && r.has_engine)
    {
        const string sub = r.electric ? formatInt(round_i(r.power)) + " % " + T(TX_POWER) : formatInt(round_i(r.rpm)) + " " + T(TX_RPM_UNIT);
        c.Text(cx, cy + 40.f, sub, 11.f, COL_TEXT);
    }
    if (r.has_engine)
        c.Text(cx, cy + 78.f, GearText(), 24.f, GearColor(GearText().substr(0, 1)));
    Needle(cx, cy, R, GA(r.speed, 0.f, r.speed_max), true);
}

// ---- Full panel ----

const float CARD_Y = 26.f;
const float CARD_H = 202.f;

float FullWidth()
{
    float w = 8.f;
    if (g_show[SH_SPEED]) w += 190.f + 8.f;
    if (g_show[SH_RPM] && r.has_engine) w += 190.f + 8.f;
    if (g_show[SH_GEAR] && r.has_engine) w += 170.f + 8.f;
    if (g_show[SH_LAMPS]) w += 236.f + 8.f;
    if (g_show[SH_LIGHTS]) w += 200.f + 8.f;
    if (g_show[SH_TYRES]) w += 190.f + 8.f;
    return fmax(w, 300.f);
}

float HeaderWidth()
{
    return c.Size(thisActor.getTruckName(), 10.f).x / c.k + 80.f;
}

/// Vehicle name, 'back to the small view' and settings
void DrawHeader(float x, float w)
{
    bool active;
    c.Rect(x, 0.f, w, 22.f, BgCol(0.035f), COL_CARD_BR, 6.f);
    c.TextL(x + 8.f, 11.f, thisActor.getTruckName(), 10.f, COL_TEXT);
    if (Area(x + w - 26.f, 0.f, 22.f, 22.f, active))
        g_open_settings = true;
    const bool hov = ImGui::IsItemHovered();
    GearIcon(x + w - 15.f, 11.f, hov);
    if (hov) ImGui::SetTooltip(Esc(T(TX_TIP_SETTINGS)));
    if (Area(x + w - 52.f, 0.f, 22.f, 22.f, active))
    {
        g_view = g_last_small_view;
        SaveSettings();
    }
    {
        const bool h2 = ImGui::IsItemHovered();
        const color col = h2 ? COL_TEXT : COL_MUTED;
        // 'shrink' icon: four inward corners
        const float ix = x + w - 41.f, iy = 11.f;
        c.Line(ix - 6.f, iy - 2.f, ix - 2.f, iy - 2.f, col, 1.5f); c.Line(ix - 2.f, iy - 6.f, ix - 2.f, iy - 2.f, col, 1.5f);
        c.Line(ix + 6.f, iy - 2.f, ix + 2.f, iy - 2.f, col, 1.5f); c.Line(ix + 2.f, iy - 6.f, ix + 2.f, iy - 2.f, col, 1.5f);
        c.Line(ix - 6.f, iy + 2.f, ix - 2.f, iy + 2.f, col, 1.5f); c.Line(ix - 2.f, iy + 6.f, ix - 2.f, iy + 2.f, col, 1.5f);
        c.Line(ix + 6.f, iy + 2.f, ix + 2.f, iy + 2.f, col, 1.5f); c.Line(ix + 2.f, iy + 6.f, ix + 2.f, iy + 2.f, col, 1.5f);
        if (h2) ImGui::SetTooltip(Esc(T(TX_TIP_SMALL)));
    }
}

void CardSpeed(float x, float w)
{
    c.Card(x, CARD_Y, w, CARD_H, T(TX_SPEED));
    const float R = 76.f, cx = x + w * 0.5f, cy = CARD_Y + 26.f + R;
    SpeedGauge(cx, cy, R, false, true, 0.27f);
    if (r.cruise >= 0.f)
    {
        c.dl.AddCircleFilled(c.P(x + 14.f, CARD_Y + CARD_H - 14.f), 3.5f * c.k, COL_WARN, 10);
        c.TextL(x + 22.f, CARD_Y + CARD_H - 14.f, T(TX_CRUISE) + " " + formatInt(round_i(r.cruise)), 9.f, COL_WARN);
    }
    else if (r.limit >= 0.f)
    {
        c.TextL(x + 12.f, CARD_Y + CARD_H - 14.f, T(TX_LIMITER) + " " + formatInt(round_i(r.limit)), 9.f, COL_MUTED);
    }
}

void CardRpm(float x, float w)
{
    c.Card(x, CARD_Y, w, CARD_H, r.electric ? T(TX_POWER) : T(TX_RPM));
    const float R = 76.f, cx = x + w * 0.5f, cy = CARD_Y + 26.f + R;
    if (r.electric)
    {
        RpmGauge(cx, cy, R, false);
        return;
    }
    Gauge(cx, cy, R, false, 0.f, r.rpm_max, r.rpm_major, r.rpm_minor, r.rpm_div, r.rpm, r.rpm_red, 0.f, g_col_rpm, R * 0.15f);
    c.Text(cx, cy + R * 0.44f, formatInt(round_i(r.rpm)), R * 0.26f, COL_TEXT);
    c.Text(cx, cy + R * 0.62f, T((r.rpm_div > 100.f) ? TX_RPM_X1000 : TX_RPM_X100), R * 0.13f, COL_MUTED);
    Needle(cx, cy, R, GA(r.rpm, 0.f, r.rpm_max), false);
    if (g_show[SH_BOOST] && r.turbo)
    {
        const float by = CARD_Y + CARD_H - 14.f;
        c.TextL(x + 12.f, by, T(TX_SH_TURBO), 9.f, COL_MUTED);
        const float bx0 = x + 54.f, bx1 = x + w - 58.f;
        c.Rect(bx0, by - 2.5f, bx1 - bx0, 5.f, COL_TRACK, Col(0, 0, 0, 0), 2.5f);
        c.Rect(bx0, by - 2.5f, (bx1 - bx0) * clampf(r.boost / r.boost_max, 0.f, 1.f), 5.f, g_col_rpm, Col(0, 0, 0, 0), 2.5f);
        c.TextR(x + w - 12.f, by, formatFloat(r.boost, "", 0, (r.boost_max > 3.f) ? 0 : 1) + " " + r.boost_unit, 9.f, COL_TEXT);
    }
}

string ModeName()
{
    switch (r.mode)
    {
    case AUTO: return T(TX_MODE_AUTO);
    case SEMI_AUTO: return T(TX_MODE_SEMI);
    case MANUAL: return T(TX_MODE_MANUAL);
    case MANUAL_STICK: return T(TX_MODE_STICK);
    }
    return T(TX_MODE_RANGES);
}

void CardGearbox(float x, float w)
{
    EngineClass@ eng = thisActor.getEngine();
    c.Card(x, CARD_Y, w, CARD_H, T(TX_GEARBOX));
    string big = GearText();
    string small = "";
    if (r.mode == AUTO)
    {
        big = big.substr(0, 1);
        if (r.gear > 0) small = formatInt(r.gear);
    }
    c.TextL(x + 12.f, CARD_Y + 46.f, big, 44.f, GearColor(big));
    const float bw = c.Size(big, 44.f).x / c.k;
    if (small != "")
        c.TextL(x + 18.f + bw, CARD_Y + 52.f, small, 22.f, COL_ACCENT);
    c.TextL(x + 18.f + bw + ((small != "") ? c.Size(small, 22.f).x / c.k + 4.f : 0.f), CARD_Y + 56.f, "/ " + r.num_gears, 10.f, COL_MUTED);

    // mode
    if (Button(x + 10.f, CARD_Y + 82.f, w - 20.f, 20.f, ModeName(), ST_OFF, KeyOf(EV_TRUCK_SWITCH_SHIFT_MODES), 8.5f))
        Pulse(EV_TRUCK_SWITCH_SHIFT_MODES);

    const float by = CARD_Y + 108.f, bh = 24.f;
    if (r.mode == AUTO)
    {
        const array<string> names = { "R", "N", "D", "2", "1" };
        const array<int> sel = { AUTOSWITCH_REAR, AUTOSWITCH_NEUTRAL, AUTOSWITCH_DRIVE, AUTOSWITCH_TWO, AUTOSWITCH_ONE };
        const int n = r.electric ? 3 : 5;
        const float bw2 = (w - 20.f - (n - 1) * 4.f) / n;
        for (int i = 0; i < n; i++)
        {
            if (Button(x + 10.f + i * (bw2 + 4.f), by, bw2, bh, names[i], (r.autoselect == sel[i]) ? ST_ON : ST_OFF, ""))
                eng.autoShiftSet(autoswitch(sel[i]));
        }
    }
    else
    {
        const float bw2 = (w - 24.f) / 3.f;
        if (Button(x + 10.f, by, bw2, bh, "R", (r.gear < 0) ? ST_ON : ST_OFF, KeyOf(EV_TRUCK_SHIFT_GEAR_REVERSE)))
            Shift(-100);
        if (Button(x + 12.f + bw2, by, bw2, bh, "N", (r.gear == 0) ? ST_ON : ST_OFF, KeyOf(EV_TRUCK_SHIFT_NEUTRAL)))
            Shift(0);
        if (Button(x + 14.f + 2.f * bw2, by, bw2, bh, "1", (r.gear == 1) ? ST_ON : ST_OFF, KeyOf(EV_TRUCK_SHIFT_GEAR01)))
            Shift(100);
    }
    const float hw = (w - 24.f) * 0.5f;
    if (Button(x + 10.f, by + bh + 6.f, hw, bh, "-", ST_OFF, KeyOf(EV_TRUCK_SHIFT_DOWN), 12.f))
        Shift(-1);
    if (Button(x + 14.f + hw, by + bh + 6.f, hw, bh, "+", ST_OFF, KeyOf(EV_TRUCK_SHIFT_UP), 12.f))
        Shift(1);

    // ranges (H-shifter with ranges; only in neutral)
    if (r.mode == MANUAL_RANGES && r.num_ranges > 1)
    {
        const array<string> rn = { "Lo", "Mid", "Hi" };
        const array<inputEvents> re = { EV_TRUCK_SHIFT_LOWRANGE, EV_TRUCK_SHIFT_MIDRANGE, EV_TRUCK_SHIFT_HIGHRANGE };
        const int n = clamp_i(r.num_ranges, 2, 3);
        const float rw = (w - 20.f - (n - 1) * 4.f) / n;
        for (int i = 0; i < n; i++)
        {
            if (Button(x + 10.f + i * (rw + 4.f), CARD_Y + CARD_H - 30.f, rw, 20.f, rn[i], (r.gear_range == i) ? ST_ON : ST_OFF, KeyOf(re[i]), 8.5f))
                Pulse(re[i]);
        }
    }
    else
    {
        c.TextL(x + 12.f, CARD_Y + CARD_H - 16.f, T(TX_RANGE), 9.f, COL_MUTED);
        c.TextR(x + w - 12.f, CARD_Y + CARD_H - 16.f, (r.num_ranges > 1) ? formatInt(r.gear_range + 1) + " / " + r.num_ranges : "-", 9.f, COL_TEXT);
    }
}

/// -100 = reverse, 0 = neutral, 100 = first gear, -1/+1 = one gear down/up
void Shift(int how)
{
    EngineClass@ eng = thisActor.getEngine();
    if (eng is null)
        return;
    if (how == -100) { eng.shiftTo(-1); return; }
    if (how == 0)    { eng.shiftTo(0); return; }
    if (how == 100)  { eng.shiftTo(1); return; }
    if (r.mode <= MANUAL)
        Pulse((how > 0) ? EV_TRUCK_SHIFT_UP : EV_TRUCK_SHIFT_DOWN); // same rules as the keys
    else
        eng.shift(how);
}

class GridButton
{
    string label;
    int state;
    int action;
    string tip;
}

const int ACT_ABS = 1, ACT_TC = 2, ACT_CRUISE = 3, ACT_LIMITER = 4, ACT_PBRAKE = 5, ACT_ENGINE = 6,
          ACT_WDIFF = 7, ACT_ADIFF = 8, ACT_TCASE = 9, ACT_TRATIO = 10,
          ACT_LOW = 20, ACT_HIGH = 21, ACT_FOG = 22, ACT_BEACON = 23, ACT_CUSTOM = 30;

void Add(array<GridButton@>@ list, const string&in label, int state, int action, const string&in tip = "")
{
    GridButton b;
    b.label = label; b.state = state; b.action = action; b.tip = tip;
    list.insertLast(b);
}

string DiffName(int type)
{
    switch (type)
    {
    case 0: return T(TX_DIFF_SPLIT);
    case 1: return T(TX_DIFF_OPEN);
    case 2: return T(TX_DIFF_VISCOUS);
    case 3: return T(TX_DIFF_LOCKED);
    }
    return "?";
}

void CardAids(float x, float w)
{
    c.Card(x, CARD_Y, w, CARD_H, T(TX_AIDS));
    array<GridButton@> b;
    Add(b, thisActor.getAntiLockBrake() ? T(TX_ABS) : T(TX_ABS_OFF), thisActor.getAntiLockBrake() ? ST_ON : ST_OFF, ACT_ABS, KeyOf(EV_TRUCK_ANTILOCK_BRAKE));
    Add(b, thisActor.getTractionControl() ? T(TX_TC) : T(TX_TC_OFF), thisActor.getTractionControl() ? ST_ON : ST_OFF, ACT_TC, KeyOf(EV_TRUCK_TRACTION_CONTROL));
    Add(b, (r.cruise >= 0.f) ? T(TX_CRUISE) + " " + formatInt(round_i(r.cruise)) : T(TX_CRUISE_OFF), (r.cruise >= 0.f) ? ST_ON : ST_OFF, ACT_CRUISE, KeyOf(EV_TRUCK_CRUISE_CONTROL));
    Add(b, (r.limit >= 0.f) ? T(TX_LIMITER) + " " + formatInt(round_i(r.limit)) : T(TX_LIMITER_OFF), (r.limit >= 0.f) ? ST_WARN : ST_OFF, ACT_LIMITER, T(TX_TIP_LIMITER));
    Add(b, T(TX_PBRAKE), thisActor.getParkingBrake() ? ST_WARN : ST_OFF, ACT_PBRAKE, KeyOf(EV_TRUCK_PARKING_BRAKE));
    if (r.has_engine)
        Add(b, r.running ? T(TX_ENGINE_ON) : T(TX_ENGINE_OFF), r.running ? ST_GREEN : ST_OFF, ACT_ENGINE, T(TX_TIP_ENGINE));
    const int wd = thisActor.getWheelDiffType();
    if (wd >= 0)
        Add(b, T(TX_DIFF) + ": " + DiffName(wd), (wd == 3) ? ST_WARN : ST_OFF, ACT_WDIFF, KeyOf(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF));
    const int ad = thisActor.getAxleDiffType();
    if (ad >= 0)
        Add(b, T(TX_AXLE_DIFF) + ": " + DiffName(ad), (ad == 3) ? ST_WARN : ST_OFF, ACT_ADIFF, KeyOf(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF));
    const int tc = thisActor.getTransferCaseState();
    if (tc >= 0)
    {
        const bool four = (tc & 1) != 0, lo = (tc & 2) != 0;
        if ((tc & 4) != 0 || (tc & 8) == 0)
            Add(b, four ? "4\u00d74" : "4\u00d72", four ? ST_ON : ST_OFF, ACT_TCASE, KeyOf(EV_TRUCK_TOGGLE_TCASE_4WD_MODE));
        if ((tc & 8) != 0)
            Add(b, lo ? "Low" : "High", lo ? ST_WARN : ST_OFF, ACT_TRATIO, KeyOf(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO));
    }
    DrawGrid(x, w, b);
}

void CardLights(float x, float w)
{
    c.Card(x, CARD_Y, w, CARD_H, T(TX_LIGHTS));
    // blinkers + hazard
    const int blink = thisActor.getBlinkType();
    const bool blink_on = (int(g_blink_time * 2.5f) % 2) == 0;
    const float bw = (w - 28.f) / 3.f, by = CARD_Y + 26.f;
    const bool left_on = blink == BLINK_LEFT && blink_on, right_on = blink == BLINK_RIGHT && blink_on, warn_on = blink == BLINK_WARN && blink_on;
    if (Button(x + 10.f, by, bw, 24.f, "", left_on ? ST_GREEN : ST_OFF, KeyOf(EV_TRUCK_BLINK_LEFT)))
        Pulse(EV_TRUCK_BLINK_LEFT);
    Arrow(x + 10.f + bw * 0.5f, by + 12.f, -1.f, COL_TEXT);
    if (Button(x + 14.f + bw, by, bw, 24.f, "", warn_on ? ST_WARN : ST_OFF, KeyOf(EV_TRUCK_BLINK_WARN)))
        Pulse(EV_TRUCK_BLINK_WARN);
    {
        const float tx = x + 14.f + bw * 1.5f, ty = by + 12.f;
        c.dl.AddTriangle(c.P(tx, ty - 6.f), c.P(tx + 7.f, ty + 6.f), c.P(tx - 7.f, ty + 6.f), COL_RED, 1.8f * c.k);
    }
    if (Button(x + 18.f + 2.f * bw, by, bw, 24.f, "", right_on ? ST_GREEN : ST_OFF, KeyOf(EV_TRUCK_BLINK_RIGHT)))
        Pulse(EV_TRUCK_BLINK_RIGHT);
    Arrow(x + 18.f + bw * 2.5f, by + 12.f, 1.f, COL_TEXT);

    array<GridButton@> b;
    Add(b, T(TX_LOW_BEAM), thisActor.getHeadlightsVisible() ? ST_ON : ST_OFF, ACT_LOW, KeyOf(EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS));
    if (thisActor.countFlaresByType(FLARE_TYPE_HIGH_BEAM) > 0)
        Add(b, T(TX_HIGH_BEAM), thisActor.getHighBeamsVisible() ? ST_ON : ST_OFF, ACT_HIGH, KeyOf(EV_COMMON_TOGGLE_TRUCK_HIGH_BEAMS));
    if (thisActor.countFlaresByType(FLARE_TYPE_FOG_LIGHT) > 0)
        Add(b, T(TX_FOG), thisActor.getFogLightsVisible() ? ST_ON : ST_OFF, ACT_FOG, KeyOf(EV_COMMON_TOGGLE_TRUCK_FOG_LIGHTS));
    if (thisActor.countBeaconProps() > 0)
        Add(b, T(TX_BEACONS), thisActor.getBeaconMode() ? ST_WARN : ST_OFF, ACT_BEACON, KeyOf(EV_COMMON_TOGGLE_TRUCK_BEACONS));
    for (int n = 0; n < MAX_LIGHT_SWITCHES && b.length() < 8; n++)
    {
        if (thisActor.countCustomLights(n) > 0)
            Add(b, T(TX_LIGHT_N) + " " + (n + 1), thisActor.getCustomLightVisible(n) ? ST_ON : ST_OFF, ACT_CUSTOM + n, KeyOf(inputEvents(int(EV_TRUCK_LIGHTTOGGLE01) + n)));
    }
    DrawGrid(x, w, b, 32.f);
}

void DrawGrid(float x, float w, array<GridButton@>@ b, float top = 0.f)
{
    const float bw = (w - 26.f) * 0.5f, bh = 24.f;
    for (uint i = 0; i < b.length(); i++)
    {
        const float bx = x + 10.f + (i % 2) * (bw + 6.f);
        const float by = CARD_Y + 26.f + top + (i / 2) * (bh + 6.f);
        if (by + bh > CARD_Y + CARD_H - 4.f)
            break;
        if (Button(bx, by, bw, bh, b[i].label, b[i].state, b[i].tip, 8.5f))
            DoAction(b[i].action);
    }
}

void DoAction(int a)
{
    EngineClass@ eng = thisActor.getEngine();
    switch (a)
    {
    case ACT_ABS: thisActor.antilockbrakeToggle(); break;
    case ACT_TC: thisActor.tractioncontrolToggle(); break;
    case ACT_CRUISE: thisActor.cruisecontrolToggle(); break;
    case ACT_PBRAKE: thisActor.parkingbrakeToggle(); break;
    case ACT_ENGINE:
        if (eng !is null)
        {
            if (r.running) eng.stopEngine();
            else eng.startEngine();
        }
        break;
    case ACT_WDIFF: Pulse(EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF); break;
    case ACT_ADIFF: Pulse(EV_TRUCK_TOGGLE_INTER_AXLE_DIFF); break;
    case ACT_TCASE: Pulse(EV_TRUCK_TOGGLE_TCASE_4WD_MODE); break;
    case ACT_TRATIO: Pulse(EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO); break;
    case ACT_LOW: Pulse(EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS); break;
    case ACT_HIGH: Pulse(EV_COMMON_TOGGLE_TRUCK_HIGH_BEAMS); break;
    case ACT_FOG: Pulse(EV_COMMON_TOGGLE_TRUCK_FOG_LIGHTS); break;
    case ACT_BEACON: Pulse(EV_COMMON_TOGGLE_TRUCK_BEACONS); break;
    default:
        if (a >= ACT_CUSTOM && a < ACT_CUSTOM + MAX_LIGHT_SWITCHES)
            Pulse(inputEvents(int(EV_TRUCK_LIGHTTOGGLE01) + (a - ACT_CUSTOM)));
        break;
    }
}

void CardTyres(float x, float w)
{
    c.Card(x, CARD_Y, w, CARD_H, T(TX_TYRES));
    const float p = thisActor.getTyrePressurePsi();
    if (p >= 0.f)
        c.TextR(x + w - 10.f, CARD_Y + 12.f, (r.imperial ? formatInt(round_i(p)) + " psi" : formatFloat(p * PSI_TO_BAR, "", 0, 1) + " bar"), 10.f, COL_TEXT);

    // chassis seen from above, driven axles highlighted
    const float cx = x + w * 0.5f, top = CARD_Y + 34.f, bot = CARD_Y + 128.f;
    c.Rect(cx - 18.f, top, 36.f, bot - top, COL_INK, COL_DARK, 8.f);
    const int tc = thisActor.getTransferCaseState();
    const bool front_driven = (tc < 0) ? true : ((tc & 1) != 0);
    const color front = front_driven ? COL_WARN : COL_DARK;
    const color rear = COL_WARN;
    c.Line(cx - 34.f, top + 14.f, cx + 34.f, top + 14.f, front_driven ? front : COL_DARK, 2.f);
    c.Line(cx - 34.f, bot - 14.f, cx + 34.f, bot - 14.f, rear, 2.f);
    c.Rect(cx - 42.f, top + 4.f, 10.f, 20.f, COL_GREEN, Col(0, 0, 0, 0), 2.f);
    c.Rect(cx + 32.f, top + 4.f, 10.f, 20.f, COL_GREEN, Col(0, 0, 0, 0), 2.f);
    c.Rect(cx - 42.f, bot - 24.f, 10.f, 20.f, (r.slip > 0.15f) ? COL_RED : COL_GREEN, Col(0, 0, 0, 0), 2.f);
    c.Rect(cx + 32.f, bot - 24.f, 10.f, 20.f, (r.slip > 0.15f) ? COL_RED : COL_GREEN, Col(0, 0, 0, 0), 2.f);
    c.TextL(x + 10.f, (top + bot) * 0.5f, (tc >= 0) ? (((tc & 1) != 0) ? T(TX_DRIVE_4WD) : T(TX_DRIVE_2WD)) : "", 8.5f, COL_MUTED);

    // slip bar
    const float sy = CARD_Y + 142.f;
    c.TextL(x + 10.f, sy, T(TX_SLIP), 8.5f, COL_MUTED);
    c.Rect(x + 70.f, sy - 2.5f, w - 120.f, 5.f, COL_TRACK, Col(0, 0, 0, 0), 2.5f);
    c.Rect(x + 70.f, sy - 2.5f, (w - 120.f) * r.slip, 5.f, (r.slip > 0.15f) ? COL_RED : COL_ACCENT, Col(0, 0, 0, 0), 2.5f);
    c.TextR(x + w - 10.f, sy, formatInt(round_i(r.slip * 100.f)) + " %", 8.5f, COL_TEXT);

    // pressure buttons (hold)
    const float by = CARD_Y + CARD_H - 34.f, bw = (w - 26.f) * 0.5f;
    if (p >= 0.f)
    {
        HoldButton(x + 10.f, by, bw, 24.f, "-", EV_COMMON_PRESSURE_LESS);
        HoldButton(x + 16.f + bw, by, bw, 24.f, "+", EV_COMMON_PRESSURE_MORE);
    }
    else
    {
        c.Text(x + w * 0.5f, by + 12.f, T(TX_PRESSURE) + ": " + T(TX_NO_PRESSURE), 8.f, COL_MUTED);
    }
}

/// Button that holds a global input event while pressed (tyre pressure uses the global input state)
void HoldButton(float x, float y, float w, float h, const string&in label, inputEvents ev)
{
    Button(x, y, w, h, label, ST_OFF, T(TX_PRESSURE) + " \u00b7 " + T(TX_TIP_HOLD) + ((KeyOf(ev) != "") ? " \u00b7 " + KeyOf(ev) : ""), 12.f);
    if (ImGui::IsItemActive())
    {
        inputs.setEventSimulatedValue(ev, 1.f);
        g_held_now.insertLast(int(ev));
    }
}

// ---- Settings popup ----

void DrawSettings()
{
    if (g_open_settings)
    {
        ImGui::OpenPopup("CarDashboardSettings");
        g_open_settings = false;
    }
    if (!ImGui::BeginPopup("CarDashboardSettings"))
        return;
    bool changed = false;
    ImGui::TextDisabled(T(TX_SETTINGS));
    ImGui::TextDisabled(T(TX_VIEW));
    int v = g_view;
    if (ImGui::RadioButton(T(TX_VIEW_ONE) + "##v0", v, VIEW_ONE)) { g_view = VIEW_ONE; g_last_small_view = VIEW_ONE; changed = true; }
    if (ImGui::RadioButton(T(TX_VIEW_TWO) + "##v1", v, VIEW_TWO)) { g_view = VIEW_TWO; g_last_small_view = VIEW_TWO; changed = true; }
    if (ImGui::RadioButton(T(TX_VIEW_FULL) + "##v2", v, VIEW_FULL)) { g_view = VIEW_FULL; changed = true; }
    ImGui::Separator();
    ImGui::TextDisabled(T(TX_SHOW));
    if (g_view == VIEW_ONE)
    {
        changed = Show(SH_RPM, TX_SH_RPM_RING, false) || changed;
        changed = Show(SH_CRUISE, TX_SH_CRUISE, false) || changed;
        changed = Show(SH_GEAR, TX_SH_GEARSTRIP, false) || changed;
        changed = Show(SH_LAMPS, TX_SH_LAMPS, false) || changed;
        changed = Show(SH_VIEWBTN, TX_SH_VIEWBTN, false) || changed;
    }
    else if (g_view == VIEW_TWO)
    {
        changed = Show(SH_RPM, TX_SH_RPM, false) || changed;
        changed = Show(SH_SPEED, TX_SH_SPEED, false) || changed;
        changed = Show(SH_CRUISE, TX_SH_CRUISE, true) || changed;
        changed = Show(SH_BOOST, TX_SH_BOOST, false) || changed;
        changed = Show(SH_PITCH, TX_SH_PITCH, false) || changed;
        changed = Show(SH_ROLL, TX_SH_ROLL, false) || changed;
        changed = Show(SH_GEAR, TX_SH_GEARSTRIP, false) || changed;
        changed = Show(SH_LAMPS, TX_SH_LAMPS, false) || changed;
        changed = Show(SH_VIEWBTN, TX_SH_VIEWBTN, false) || changed;
    }
    else
    {
        changed = Show(SH_SPEED, TX_SH_SPEED, false) || changed;
        changed = Show(SH_CRUISE, TX_SH_CRUISE, true) || changed;
        changed = Show(SH_RPM, TX_SH_RPM, false) || changed;
        changed = Show(SH_BOOST, TX_SH_TURBO, true) || changed;
        changed = Show(SH_GEAR, TX_SH_GEARBOX, false) || changed;
        changed = Show(SH_LAMPS, TX_SH_AIDS, false) || changed;
        changed = Show(SH_LIGHTS, TX_SH_LIGHTS, false) || changed;
        changed = Show(SH_TYRES, TX_SH_TYRES, false) || changed;
    }
    ImGui::Separator();
    ImGui::TextDisabled(T(TX_UNITS));
    int u = g_units;
    if (ImGui::RadioButton(T(TX_UNITS_GAME) + "##u0", u, 0)) { g_units = 0; changed = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("km/h##u1", u, 1)) { g_units = 1; changed = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("mph##u2", u, 2)) { g_units = 2; changed = true; }
    ImGui::Separator();
    ImGui::TextDisabled(T(TX_COLORS));
    ColorField("colspeed", T(TX_COL_SPEED), g_col_speed);
    ColorField("colrpm", T(TX_COL_RPM), g_col_rpm);
    ColorField("colbg", T(TX_COL_BG), g_col_bg);
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::SliderInt(T(TX_BG_OPACITY) + "##bgpct", g_bg_pct, 0, 100)) g_save_pending = true;
    if (ImGui::Button(T(TX_COL_RESET) + "##colreset"))
    {
        g_col_speed = COL_ACCENT;
        g_col_rpm = COL_ACCENT;
        g_col_bg = COL_DIAL_DEF;
        g_bg_pct = BG_PCT_DEF;
        changed = true;
    }
    ImGui::Separator();
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::SliderFloat(T(TX_SIZE) + "##size", g_scale_mul, 0.5f, 2.f)) changed = true;
    if (ImGui::Checkbox(T(TX_ARRANGE) + "##arrange", g_arrange)) changed = true;
    if (g_arrange)
    {
        ImGui::Indent(16.f);
        if (ImGui::Checkbox(T(TX_SNAP) + "##snap", g_snap)) changed = true;
        ImGui::TextDisabled(T(TX_ARRANGE_HINT));
        ImGui::Unindent(16.f);
    }
    if (ImGui::Button(T(TX_RESET_LAYOUT) + "##resetlayout"))
    {
        ResetLayout(g_view);
        changed = true;
    }
    ImGui::Separator();
    ImGui::TextDisabled(T(TX_OTHER_DASH));
    if (changed)
        SaveSettings();
    ImGui::EndPopup();
}

/// Colour editor (colour square + R, G, B); a click on the square opens the colour picker. Saved when the mouse is released.
void ColorField(const string&in id, const string&in label, color&inout col)
{
    ImGui::SetNextItemWidth(200.f);
    if (ImGui::ColorEdit3(label + "##" + id, col))
        g_save_pending = true;
}

bool Show(int idx, Txt label, bool indent)
{
    if (indent) ImGui::Indent(16.f);
    bool v = g_show[idx];
    const bool changed = ImGui::Checkbox(T(label) + "##show" + idx, v);
    if (changed) g_show[idx] = v;
    if (indent) ImGui::Unindent(16.f);
    return changed;
}

// ---- Main ----

void frameStep(float dt)
{
    if (thisActor is null || g_storage is null)
        return;
    ReleaseInputs();
    BeamClass@ player = game.getCurrentTruck();
    if (player is null || player.getInstanceId() != thisActor.getInstanceId())
        return;
    DashBoardManagerClass@ dashboard = thisActor.getDashboardManager();
    if (dashboard is null || !dashboard.getVisible())
        return;
    if (thisActor.getNodeCount() == 0)
        return;

    Read(dt);
    if (g_save_pending && !ImGui::IsMouseDown(0))
    {
        g_save_pending = false;
        SaveSettings();
    }

    const vector2 scr = game.getDisplaySize();
    const vector2 ds = ViewSize();
    const float k = fmax(0.4f, fmin(scr.y / 1300.f * g_scale_mul, scr.x * 0.98f / ds.x));
    c.k = k;
    BuildLayout(ds.x, ds.y);

    // Concept place of the whole view: small views bottom right, full panel bottom center; plus the user's offsets
    vector2 base(scr.x - ds.x * k - 8.f, scr.y - ds.y * k - 8.f);
    if (g_view == VIEW_FULL)
        base = vector2((scr.x - ds.x * k) * 0.5f, scr.y - ds.y * k - 8.f);
    base = base + vector2(g_grp_x[g_view] * k, g_grp_y[g_view] * k);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, vector2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, color(0.f, 0.f, 0.f, 0.f));
    const int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    const int vi = g_view * EL_COUNT;
    vector2 done_pos(scr.x * 0.5f, 8.f); // 'Done' button while arranging: above (or below) the settings button
    float done_bottom = 8.f;
    for (uint n = 0; n < g_elems.length(); n++)
    {
        Elem@ e = g_elems[n];
        const float pad = g_arrange ? 3.f : 0.f; // room for the frame
        float ox = g_off_x[vi + e.id], oy = g_off_y[vi + e.id];
        // keep it on the screen
        const float sx = base.x + (e.x + ox - pad) * k, sy = base.y + (e.y + oy - pad) * k;
        const float sw = (e.w + 2.f * pad) * k, sh = (e.h + 2.f * pad) * k;
        const float cx = clampf(sx, 0.f, fmax(0.f, scr.x - sw)), cy = clampf(sy, 0.f, fmax(0.f, scr.y - sh));
        ox += (cx - sx) / k; oy += (cy - sy) / k;

        if (e.id == EL_BUTTONS || e.id == EL_HEADER)
            { done_pos = vector2(cx + sw, cy); done_bottom = cy + sh; }
        ImGui::SetNextWindowPos(vector2(cx, cy), ImGuiCond_Always);
        ImGui::SetNextWindowSize(vector2(sw, sh));
        bool window_open = true;
        if (ImGui::Begin("CarDashboard##" + e.id, window_open, flags))
        {
            @c.dl = ImGui::GetWindowDrawList();
            c.o = vector2(base.x + ox * k, base.y + oy * k); // layout coordinates -> screen
            g_idc = 0;
            bool active;
            if (g_arrange)
            {
                // the frame is submitted first, so it takes the clicks instead of the buttons
                Area(e.x - pad, e.y - pad, e.w + 2.f * pad, e.h + 2.f * pad, active);
                const bool hov = ImGui::IsItemHovered() || active;
                if (ImGui::IsItemClicked(1)) // right click ends arranging
                {
                    g_arrange = false;
                    g_drag_el = 99; // saves
                }
                DrawElement(e);
                DashedRect(e.x - pad + 1.f, e.y - pad + 1.f, e.w + 2.f * pad - 2.f, e.h + 2.f * pad - 2.f, hov ? COL_TEXT : g_col_speed);
                if (active && ImGui::IsMouseDragging(0, 1.f))
                {
                    if (g_drag_el != e.id) { g_drag_el = e.id; g_drag_x = g_off_x[vi + e.id]; g_drag_y = g_off_y[vi + e.id]; }
                    const vector2 d = ImGui::GetMouseDragDelta(0, 1.f);
                    ImGui::ResetMouseDragDelta(0);
                    g_drag_x += d.x / k; g_drag_y += d.y / k;
                    g_off_x[vi + e.id] = g_snap ? round_i(g_drag_x / GRID) * GRID : g_drag_x;
                    g_off_y[vi + e.id] = g_snap ? round_i(g_drag_y / GRID) * GRID : g_drag_y;
                }
            }
            else
            {
                DrawElement(e);
                // dragging a free spot moves the whole dashboard (submitted last, so the buttons keep their clicks)
                Area(e.x, e.y, e.w, e.h, active);
                if (active && ImGui::IsMouseDragging(0, 1.f))
                {
                    const vector2 d = ImGui::GetMouseDragDelta(0, 1.f);
                    ImGui::ResetMouseDragDelta(0);
                    g_grp_x[g_view] += d.x / k; g_grp_y[g_view] += d.y / k;
                    g_drag_el = 99;
                }
            }
            if (e.id == EL_BUTTONS || e.id == EL_HEADER)
                DrawSettings();
        }
        ImGui::End();
    }
    if (g_arrange)
        DoneButton(done_pos, done_bottom, k);
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
    if (g_drag_el >= 0 && !ImGui::IsMouseDown(0))
    {
        g_drag_el = -1;
        SaveSettings();
    }
}

/// While arranging: 'Done' button (right aligned at pos, above it), ends the arrange mode
void DoneButton(const vector2&in pos, float bottom, float k)
{
    const string label = T(TX_ARRANGE_DONE);
    const float w = c.Size(label, 10.f).x / k + 24.f, h = 24.f;
    const float wx = clampf(pos.x - w * k, 0.f, game.getDisplaySize().x - w * k);
    float wy = pos.y - (h + 6.f) * k;
    if (wy < 0.f)
        wy = bottom + 6.f * k; // no room above: below the settings button
    ImGui::SetNextWindowPos(vector2(wx, wy), ImGuiCond_Always);
    ImGui::SetNextWindowSize(vector2(w * k, h * k));
    const int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
    bool window_open = true;
    if (ImGui::Begin("CarDashboard##done", window_open, flags))
    {
        @c.dl = ImGui::GetWindowDrawList();
        c.o = ImGui::GetWindowPos();
        g_idc = 0;
        if (Button(0.f, 0.f, w, h, label, ST_ON, T(TX_ARRANGE_DONE_TIP), 10.f))
        {
            g_arrange = false;
            SaveSettings();
        }
    }
    ImGui::End();
}

void DashedRect(float x, float y, float w, float h, const color&in col)
{
    const float dash = 6.f, gap = 4.f;
    for (float t = 0.f; t < w; t += dash + gap)
    {
        c.Line(x + t, y, x + fmin(t + dash, w), y, col, 1.5f);
        c.Line(x + t, y + h, x + fmin(t + dash, w), y + h, col, 1.5f);
    }
    for (float t = 0.f; t < h; t += dash + gap)
    {
        c.Line(x, y + t, x, y + fmin(t + dash, h), col, 1.5f);
        c.Line(x + w, y + t, x + w, y + fmin(t + dash, h), col, 1.5f);
    }
}

/// Back to the concept arrangement of this view (place and order of the parts)
void ResetLayout(int view)
{
    for (int e = 0; e < EL_COUNT; e++)
    {
        g_off_x[view * EL_COUNT + e] = 0.f;
        g_off_y[view * EL_COUNT + e] = 0.f;
    }
    g_grp_x[view] = 0.f;
    g_grp_y[view] = 0.f;
}

void main()
{
    InitTexts();
    LoadSettings();
}
