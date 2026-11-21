// Built-in airplane dashboard of Rigs of Rods, drawn with DearIMGUI from AngelScript.
// Loaded by 'imgui_airplane.dashboard' (a dashboard without '.layout' file), see DashBoardManager::loadDashBoard().
// Movable by dragging the background; gear button: choose instruments, scale, reset position.
// Layout is in "design units", scaled by the user-adjustable factor.
// ---------------------------------------------------------------------------------------------------------------

// ---- Constants ----

const float PI_F = 3.14159265f;
const float DEG2RAD = PI_F / 180.f;
const int   MAX_ENGINES = 8;

color Col(int r, int g, int b, int a = 255) { return color(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }

const color COL_RING    = Col(255, 255, 255, 70);
const color COL_TICK    = Col(242, 244, 247);
const color COL_TEXT    = Col(230, 233, 238);
const color COL_MUTED   = Col(154, 161, 171);
const color COL_ACC_DEF = Col(239, 159, 39);  // default accent (needles)
const color COL_BG_DEF  = Col(18, 20, 23);    // default background
const int   BG_PCT_DEF  = 88;                 // default background opacity [%]
const color COL_PILL    = Col(255, 255, 255, 26);
const color COL_BLUE    = Col(55, 138, 221);
const color COL_GREEN   = Col(29, 158, 117);
const color COL_RED     = Col(226, 75, 74);
const color COL_SKY     = Col(122, 166, 214);
const color COL_GROUND  = Col(138, 90, 60);
const color COL_WHITE   = Col(255, 255, 255);
const color COL_OAT     = Col(55, 138, 221, 46);

// ---- Small helpers ----

float fmin(float a, float b) { return (a < b) ? a : b; }
float fmax(float a, float b) { return (a > b) ? a : b; }
float clampf(float v, float lo, float hi) { return fmin(fmax(v, lo), hi); }
int   trunc_i(float v) { return int(v); }
int   round_i(float v) { return int(floor(v + 0.5f)); }

// ---- Settings (kept in 'imgui_dashboards.asdata') ----

array<bool> g_show = { true, true, true, true, true, true, true, true, true }; // 0..5: airspeed, altitude, vertical speed, attitude, AOA, heading; 6..8: engine RPM, pitch, torque
bool  g_show_temp = true;
float g_scale_mul = 1.f;
color g_col_acc = COL_ACC_DEF;
color g_col_bg = COL_BG_DEF;
int   g_bg_pct = BG_PCT_DEF;
LocalStorageClass@ g_storage;

void LoadSettings()
{
    @g_storage = LocalStorageClass("imgui_dashboards");
    for (uint i = 0; i < g_show.length(); i++)
    {
        if (g_storage.exists("airplane_show" + i))
            g_show[i] = g_storage.getBool("airplane_show" + i);
    }
    if (g_storage.exists("airplane_show_temp"))   g_show_temp = g_storage.getBool("airplane_show_temp");
    LoadLayout();
    if (g_storage.exists("airplane_scale"))       g_scale_mul = clampf(g_storage.getFloat("airplane_scale"), 0.6f, 2.f);
}

void SaveSettings()
{
    for (uint i = 0; i < g_show.length(); i++)
        g_storage.setBool("airplane_show" + i, g_show[i]);
    g_storage.setBool("airplane_show_temp", g_show_temp);
    SaveLayout();
    g_storage.setFloat("airplane_scale", g_scale_mul);
    g_storage.save();
}

// ---- State ----

float g_hold_timer = 0.f;
float g_dt = 0.f;
int   g_edit_field = -1;     // autopilot value being typed in: 0 HDG, 1 ALT, 2 VS, 3 IAS; -1 none
bool  g_edit_focus = false;
bool  g_edit_was_active = false;
string g_edit_buf;
int   g_idc = 0;             // ImGui IDs of the invisible buttons, restarted every frame

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
    void TextL(float x, float cy, const string&in s, float size, const color&in col) // left aligned
    {
        const vector2 ts = Size(s, size);
        const vector2 p = P(x, cy);
        dl.AddText(vector2(p.x, p.y - ts.y * 0.5f), col, s, size * k);
    }
    void TextLR(float cx, float cy, const string&in a, const string&in b, float size) // "name value", centered together
    {
        const vector2 sa = Size(a, size);
        const vector2 sb = Size(b, size);
        const vector2 p = P(cx, cy);
        const float x0 = p.x - (sa.x + sb.x) * 0.5f;
        const float y0 = p.y - sa.y * 0.5f;
        dl.AddText(vector2(x0, y0), COL_MUTED, a, size * k);
        dl.AddText(vector2(x0 + sa.x, y0), COL_WHITE, b, size * k);
    }
    void Line(float x1, float y1, float x2, float y2, const color&in col, float w)
    {
        dl.AddLine(P(x1, y1), P(x2, y2), col, w * k);
    }
}

Ctx c;

// Round gauge ('half' = left half disc, pivot at the right edge).
// The scale maps value -> angle (deg clockwise from top): see the 'Angle*' functions.
funcdef float AngleFunc(float);

float AngleAirspeed(float v)  { return -140.f + clampf(v, 0.f, g_asi_full) / g_asi_full * 280.f; }
float AngleAltimeter(float v) { return v * 36.f; }
float AngleVsi(float v)
{
    const float s = (v < 0.f) ? -1.f : 1.f;
    const float a = fmin(6.5f, abs(v));
    const float deg = (a <= 4.f) ? a * 12.5f : 50.f + (a - 4.f) * 10.f;
    return -90.f + s * deg;
}
float AngleAoa(float v)       { return -90.f + clampf(v, -24.f, 24.f) * 3.5f; }
float AngleRpm(float v)       { return -130.f + clampf(v, 0.f, 110.f) / 110.f * 260.f; }
float AnglePitch(float v)     { return clampf(v, -65.f, 65.f) * 2.f; }

float g_asi_full = 320.f; // full scale of the airspeed dial [kt], see AirspeedScaleKt()

void Dial(float cx, float cy, float r, bool half, AngleFunc@ f, const array<float>&in tick_vals, const array<string>&in tick_labels, float needle_val, float fs)
{
    if (half)
    {
        c.dl.PathClear();
        c.dl.PathArcTo(c.P(cx, cy), r * c.k, PI_F * 0.5f, PI_F * 1.5f, 32);
        c.dl.PathFillConvex(DialBg());
        c.dl.PathArcTo(c.P(cx, cy), r * c.k, PI_F * 0.5f, PI_F * 1.5f, 32);
        c.dl.PathStroke(COL_RING, true, 1.f);
    }
    else
    {
        c.dl.AddCircleFilled(c.P(cx, cy), r * c.k, DialBg(), 48);
        c.dl.AddCircle(c.P(cx, cy), r * c.k, COL_RING, 48, 1.f);
    }
    for (uint i = 0; i < tick_vals.length(); i++)
    {
        const float a = f(tick_vals[i]);
        c.dl.AddLine(c.Pol(cx, cy, r - 1.5f, a), c.Pol(cx, cy, r - 7.f, a), COL_TICK, 1.5f * c.k);
        if (tick_labels[i] != "")
            c.TextAt(c.Pol(cx, cy, r - 15.f, a), tick_labels[i], fs, COL_TEXT);
    }
    const float na = f(needle_val);
    c.dl.AddLine(c.Pol(cx, cy, -r * 0.1f, na), c.Pol(cx, cy, r - 10.f, na), g_col_acc, 3.f * c.k);
    c.dl.AddCircleFilled(c.P(cx, cy), 3.5f * c.k, g_col_acc, 12);
}

/// Full scale of the airspeed dial [kt], from the aircraft itself:
/// the estimated top speed at sea level (engine power or thrust against the size of the airframe; drag ~ mass^(2/3))
/// and the fastest speed reached so far, with some headroom, rounded up to a readable scale.
float g_top_speed_kt = 0.f;
float AirspeedScaleKt(float current_kt)
{
    g_top_speed_kt = fmax(g_top_speed_kt, current_kt);
    const float mass = fmax(thisActor.getTotalMass(true), 100.f);
    const float area = pow(mass, 2.f / 3.f);
    float prop_w = 0.f, jet_n = 0.f;
    const int n = thisActor.getAircraftEngineCount();
    for (int i = 0; i < n; i++)
    {
        TurbopropClass@ tp = thisActor.getTurboprop(i);
        if (tp !is null)
            prop_w += tp.getPropellerMaxPower() * 1000.f;
        TurbojetClass@ tj = thisActor.getTurbojet(i);
        if (tj !is null)
            jet_n += fmax(tj.getMaxDryThrust(), tj.getAfterburnerThrust()) * 1000.f;
    }
    // Calibrated with a Cessna 172 (120 kW, 1100 kg: ~125 kt) and jets between an airliner and a fighter
    const float v_prop = 6.f * pow(prop_w / area, 1.f / 3.f);
    const float v_jet = 18.f * sqrt(jet_n / area);
    const float est_kt = fmax(v_prop, v_jet) * 1.9438f;
    const float need = fmax(est_kt * 1.25f, g_top_speed_kt * 1.1f);
    const array<float> scales = { 120.f, 160.f, 200.f, 250.f, 320.f, 400.f, 500.f, 640.f, 800.f, 1000.f, 1250.f, 1600.f, 2000.f };
    for (uint i = 0; i < scales.length(); i++)
    {
        if (scales[i] >= need)
            return scales[i];
    }
    return scales[scales.length() - 1];
}

// ---- Clickable areas ----

bool Hit(float x, float y, float w, float h)
{
    ImGui::SetCursorScreenPos(c.P(x, y));
    ImGui::PushID(g_idc++);
    ImGui::InvisibleButton("b", vector2(w * c.k, h * c.k));
    ImGui::PopID();
    return ImGui::IsItemClicked(0);
}

/// Like Hit(), but repeats while held down
bool Repeat(float x, float y, float w, float h)
{
    ImGui::SetCursorScreenPos(c.P(x, y));
    ImGui::PushID(g_idc++);
    ImGui::InvisibleButton("b", vector2(w * c.k, h * c.k));
    ImGui::PopID();
    if (ImGui::IsItemClicked(0))
    {
        g_hold_timer = 0.35f;
        return true;
    }
    if (ImGui::IsItemActive())
    {
        g_hold_timer -= g_dt;
        if (g_hold_timer <= 0.f)
        {
            g_hold_timer = 0.08f;
            return true;
        }
    }
    return false;
}

bool Pill(float x, float w, const string&in txt, bool active, const color&in col)
{
    c.dl.AddRectFilled(c.P(x, 12.f), c.P(x + w, 28.f), active ? col : COL_PILL, 8.f * c.k);
    c.Text(x + w * 0.5f, 20.f, txt, 8.f, active ? COL_WHITE : COL_TEXT);
    return Hit(x, 12.f, w, 16.f);
}

/// Label, value, up/down arrows. Returns +1/-1 while an arrow is pressed.
int ApGroup(float lx, const string&in label, float vx, const string&in val, float ax)
{
    c.TextL(lx, 20.f, label, 9.f, COL_MUTED);
    c.TextL(vx, 20.f, val, 11.f, COL_WHITE);
    c.dl.AddTriangleFilled(c.P(ax + 4.f, 12.f), c.P(ax, 18.f), c.P(ax + 8.f, 18.f), COL_TEXT);
    c.dl.AddTriangleFilled(c.P(ax, 22.f), c.P(ax + 8.f, 22.f), c.P(ax + 4.f, 28.f), COL_TEXT);
    int res = 0;
    if (Repeat(ax - 1.f, 11.f, 10.f, 9.f)) res = 1;
    if (Repeat(ax - 1.f, 20.f, 10.f, 9.f)) res = -1;
    return res;
}

/// Click a value to type it in with the keyboard (Enter applies, Esc/click elsewhere cancels).
/// Returns true and the typed number when the user confirmed.
bool EditValue(int field, float vx, float vw, int current, int &out typed)
{
    typed = 0;
    ImGui::SetCursorScreenPos(c.P(vx - 2.f, 11.f));
    if (g_edit_field != field)
    {
        ImGui::PushID(g_idc++);
        ImGui::InvisibleButton("v", vector2((vw + 4.f) * c.k, 18.f * c.k));
        ImGui::PopID();
        // start on button release, so the mouse button is no longer held by this widget
        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(0))
        {
            g_edit_field = field;
            g_edit_focus = true;
            g_edit_was_active = false;
            g_edit_buf = formatInt(current);
        }
        return false;
    }
    g_idc++; // keep the ids of the following buttons stable
    bool done = false;
    ImGui::SetCursorScreenPos(c.P(vx - 3.f, 11.f));
    ImGui::SetNextItemWidth((vw + 6.f) * c.k);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, vector2(2.f * c.k, 1.f * c.k));
    ImGui::SetWindowFontScale(11.f * c.k / ImGui::GetTextLineHeight());
    if (g_edit_focus)
        ImGui::SetKeyboardFocusHere(0); // repeated until the field really is active
    ImGui::PushID(2000 + field);
    ImGui::InputText("##edit", g_edit_buf);
    const bool active = ImGui::IsItemActive();
    if (active)
    {
        g_edit_was_active = true;
        g_edit_focus = false;
    }
    else if (g_edit_was_active)
    {
        // Enter deactivates the field: apply. Esc or a click elsewhere: cancel.
        if (inputs.isKeyDown(KC_RETURN))
        {
            typed = parseInt(g_edit_buf);
            done = true;
        }
        g_edit_field = -1;
    }
    else if (ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered())
    {
        g_edit_field = -1; // never got the focus
    }
    ImGui::PopID();
    ImGui::SetWindowFontScale(1.f);
    ImGui::PopStyleVar(1);
    return done;
}

string Pad3(int v) { return formatInt(v, "0", 3); }

// ---- Main ----

void frameStep(float dt)
{
    g_dt = dt;
    if (thisActor is null || g_storage is null)
        return;
    BeamClass@ player = game.getCurrentTruck();
    if (player is null || player.getInstanceId() != thisActor.getInstanceId())
        return;
    DashBoardManagerClass@ dashboard = thisActor.getDashboardManager();
    if (dashboard is null || !dashboard.getVisible())
        return;
    const int num_nodes = thisActor.getNodeCount();
    const int n = int(fmin(float(thisActor.getAircraftEngineCount()), float(MAX_ENGINES)));
    if (n == 0 || num_nodes == 0)
        return;
    @g_ap = thisActor.getAutopilot();
    g_n = n;

    // ---- Flight data ----
    g_altitude = thisActor.getNodePosition(0).y;
    g_alt_ft = g_altitude * 3.28084f;
    g_velo = thisActor.getNodeVelocity(0);
    const float ground_speed_kt = g_velo.length() * 1.9438f;
    const float airpressure = 101325.f * pow(1.f - 0.0065f * g_altitude / 288.15f, 5.24947f);
    g_kt = ground_speed_kt * sqrt(airpressure * 0.0000120896f / 1.225f);
    g_dirv = thisActor.getDirection();
    g_hdg_value = (g_ap !is null) ? g_ap.getHeadingValue() : 0;

    // ---- Layout (design units) ----
    const float FULL_W = 120.f, HALF_W = 64.f;
    const array<float> slot_w = { FULL_W, FULL_W, HALF_W, FULL_W, HALF_W, FULL_W };
    g_elems.resize(0);
    AddEl(EL_STRIP, 8.f, 8.f, STRIP_END - 8.f, 24.f);
    float x = 8.f;
    for (int i = 0; i < 6; i++)
    {
        if (!g_show[i])
            continue;
        AddEl(EL_ASI + i, x, 36.f, slot_w[i], 162.f);
        x += slot_w[i];
    }
    g_has_dials = g_show[6] || g_show[7] || g_show[8];
    g_col_w = g_has_dials ? 62.f : 36.f;
    const float eng_x = fmax(x, STRIP_END - 4.f) + 6.f;
    AddEl(EL_ENGINES, eng_x, 4.f, g_col_w * n + 4.f, 194.f);
    const float DW = eng_x + g_col_w * n + 8.f;
    const float DH = 200.f;

    const vector2 scr = game.getDisplaySize();
    const float k = fmax(0.5f, fmin((scr.x * 0.5f / 770.f) * g_scale_mul, scr.x * 0.97f / DW));
    RunLayout(vector2((scr.x - DW * k) * 0.5f, scr.y - DH * k - 8.f), k, true);
}

// ---- Elements ----

const int EL_STRIP = 0;      // autopilot, outside temperature, settings
const int EL_ASI = 1;        // airspeed, altitude, vertical speed, attitude, AOA, HSI: EL_ASI + 0..5
const int EL_ENGINES = 7;
const int EL_COUNT = 8;
const int EL_SETTINGS = EL_STRIP; // element with the settings button (and popup)
const float STRIP_END = 630.f;

AutopilotClass@ g_ap;
int   g_n = 0;
float g_altitude = 0.f, g_alt_ft = 0.f, g_kt = 0.f, g_col_w = 62.f;
vector3 g_velo, g_dirv;
int   g_hdg_value = 0;
bool  g_has_dials = true;

/// Faces of the round instruments: a bit lighter than the background
color DialBg()
{
    return BgCol(0.045f);
}

void DrawElement(const Elem@ e)
{
    if (e.id == EL_STRIP)
        DrawStrip();
    else if (e.id == EL_ENGINES)
    {
        for (int i = 0; i < g_n; i++)
            DrawEngine(i, e.x + g_col_w * i, g_has_dials);
    }
    else
        DrawInstrument(e.id - EL_ASI, e.x);
}

void DrawStrip()
{
    const float k = c.k;
    AutopilotClass@ ap = g_ap;
    const int hdg_value = g_hdg_value;
    const int alt_value = (ap !is null) ? ap.getAltitudeValue() : 1000;
    const int vs_value = (ap !is null) ? ap.getVerticalSpeedValue() : 0;
    const int ias_value = (ap !is null) ? ap.getIASValue() : 150;
    const APHeadingMode hdg_mode = (ap !is null) ? ap.getHeadingMode() : HEADING_NONE;
    const APAltitudeMode alt_mode = (ap !is null) ? ap.getAltitudeMode() : ALT_NONE;
    const bool ias_mode = (ap !is null) && ap.getIASMode();
    const bool gpws_mode = (ap !is null) && ap.getGPWSMode();

    c.dl.AddRectFilled(c.P(8.f, 8.f), c.P(STRIP_END, 32.f), COL_PILL, 12.f * k);
    int r;
    int typed = 0;
    r = ApGroup(18.f, "HDG", 40.f, Pad3(hdg_value), 64.f);
    if (ap !is null) { if (r == 1) ap.adjustHeading(1); else if (r == -1) ap.adjustHeading(-1); }
    if (EditValue(0, 40.f, 22.f, hdg_value, typed) && ap !is null)
        ap.adjustHeading(((typed % 360) + 360) % 360 - ap.getHeadingValue());
    if (Pill(82.f, 28.f, "HDG", hdg_mode == HEADING_FIXED, COL_BLUE) && ap !is null) ap.toggleHeading(HEADING_FIXED);
    if (Pill(114.f, 28.f, "WLV", hdg_mode == HEADING_WLV, COL_BLUE) && ap !is null) ap.toggleHeading(HEADING_WLV);
    if (Pill(146.f, 28.f, "NAV", hdg_mode == HEADING_NAV, COL_BLUE) && ap !is null) ap.toggleHeading(HEADING_NAV);

    r = ApGroup(186.f, "ALT", 206.f, formatInt(alt_value / 100) + "00", 240.f);
    if (ap !is null) { if (r == 1) ap.adjustAltitude(100); else if (r == -1) ap.adjustAltitude(-100); }
    if (EditValue(1, 206.f, 32.f, alt_value, typed) && ap !is null)
        ap.adjustAltitude(int(fmax(0.f, float(typed))) - ap.getAltitudeValue());
    if (Pill(256.f, 32.f, "HOLD", alt_mode == ALT_FIXED, COL_BLUE) && ap !is null) ap.toggleAltitude(ALT_FIXED);

    const string vs_text = (vs_value == 0) ? "000" : formatInt(vs_value / 100, "+") + "00";
    r = ApGroup(298.f, "VS", 312.f, vs_text, 338.f);
    if (ap !is null) { if (r == 1) ap.adjustVerticalSpeed(100); else if (r == -1) ap.adjustVerticalSpeed(-100); }
    if (EditValue(2, 312.f, 24.f, vs_value, typed) && ap !is null)
        ap.adjustVerticalSpeed(typed - ap.getVerticalSpeedValue());
    if (Pill(354.f, 24.f, "VS", alt_mode == ALT_VS, COL_BLUE) && ap !is null) ap.toggleAltitude(ALT_VS);

    r = ApGroup(388.f, "IAS", 408.f, Pad3(ias_value), 434.f);
    if (ap !is null) { if (r == 1) ap.adjustIAS(1); else if (r == -1) ap.adjustIAS(-1); }
    if (EditValue(3, 408.f, 24.f, ias_value, typed) && ap !is null)
        ap.adjustIAS(typed - ap.getIASValue());
    if (Pill(450.f, 30.f, "ATHR", ias_mode, COL_BLUE) && ap !is null) ap.toggleIAS();
    if (Pill(486.f, 32.f, "GPWS", gpws_mode, COL_GREEN) && ap !is null) ap.toggleGPWS();
    if (Pill(522.f, 32.f, "BRKS", thisActor.getParkingBrake(), COL_RED)) thisActor.parkingbrakeToggle();

    // Outside air temperature (ISA standard atmosphere, 15 C at sea level, -6.5 C per 1000 m)
    if (g_show_temp)
    {
        const float oat = 15.f - 0.0065f * fmin(g_altitude, 11000.f);
        c.dl.AddRectFilled(c.P(560.f, 12.f), c.P(608.f, 28.f), COL_OAT, 8.f * k);
        c.Text(584.f, 20.f, "OAT " + formatInt(round_i(oat)) + " C", 8.f, COL_WHITE);
    }
    // Settings gear
    {
        const float gx = 618.f, gy = 20.f;
        c.dl.AddCircle(c.P(gx, gy), 3.5f * k, COL_TEXT, 16, 1.6f * k);
        for (int i = 0; i < 8; i++)
            c.dl.AddLine(c.Pol(gx, gy, 3.5f, i * 45.f), c.Pol(gx, gy, 6.f, i * 45.f), COL_TEXT, 2.f * k);
        if (Hit(gx - 9.f, gy - 9.f, 18.f, 18.f))
            ImGui::OpenPopup("AirDashSettings");
    }
}

void DrawInstrument(int which, float x)
{
    const float cy = 104.f;
    const float R = 58.f, RH = 52.f;
    const float kt = g_kt;
    // Airspeed
    if (which == 0)
    {
        const float cx = x + 120.f * 0.5f;
        g_asi_full = AirspeedScaleKt(kt);
        float step = 20.f;
        const array<float> steps = { 20.f, 25.f, 40.f, 50.f, 100.f, 200.f, 250.f, 400.f };
        for (uint i = 0; i < steps.length(); i++)
        {
            step = steps[i];
            if (g_asi_full / step <= 8.f)
                break;
        }
        array<float> tv;
        array<string> tl;
        for (float v = 0.f; v <= g_asi_full + 0.5f; v += step)
        {
            tv.insertLast(v);
            tl.insertLast(formatInt(int(v)));
        }
        Dial(cx, cy, R, false, AngleAirspeed, tv, tl, kt, 10.f);
        Under(cx, "AIRSPEED", formatInt(int(kt)) + " kt");
    }
    // Altimeter (one revolution = 1000 ft, scale in hundreds of feet)
    else if (which == 1)
    {
        const float cx = x + 120.f * 0.5f;
        const array<float> tv = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        const array<string> tl = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
        const float a1000 = g_alt_ft - 1000.f * floor(g_alt_ft / 1000.f); // 0..1000, also below 0 ft
        Dial(cx, cy, R, false, AngleAltimeter, tv, tl, a1000 / 100.f, 10.f);
        Under(cx, "ALTITUDE x100", formatInt(int(g_alt_ft)) + " ft");
    }
    // Vertical speed (half dial, 0 at 9 o'clock, scale in 1000 ft/min)
    else if (which == 2)
    {
        const float px = x + 64.f - 6.f;
        const array<float> tv = { -6, -4, -2, 0, 2, 4, 6 };
        const array<string> tl = { "6", "4", "2", "0", "2", "4", "6" };
        const float vvi = g_velo.y * 196.85f;
        Dial(px, cy, RH, true, AngleVsi, tv, tl, vvi / 1000.f, 9.f);
        Under(px - 22.f, "VERT SPEED", formatInt(int(vvi), "+") + " fpm");
    }
    // Attitude indicator
    else if (which == 3)
    {
        DrawAttitude(x + 120.f * 0.5f, cy, R, g_dirv);
    }
    // AOA (half dial)
    else if (which == 4)
    {
        const float px = x + 64.f - 6.f;
        float a = thisActor.getAircraftWingAOA();
        if (kt < 10.f) a = 0.f;
        const array<float> tv = { -20, -10, 0, 10, 20 };
        const array<string> tl = { "-20", "-10", "0", "10", "20" };
        Dial(px, cy, RH, true, AngleAoa, tv, tl, a, 9.f);
        Under(px - 22.f, "AOA", formatFloat(a, "", 0, 1));
    }
    // HSI
    else if (which == 5)
    {
        DrawHsi(x + 120.f * 0.5f, cy, R, g_dirv, g_ap, g_hdg_value);
    }
}

void DrawSettings()
{
    if (!ImGui::BeginPopup("AirDashSettings"))
        return;
    const array<string> names = { "Airspeed", "Altitude", "Vertical speed", "Attitude", "AOA", "Heading (HSI)",
                                  "Engine RPM", "Engine pitch", "Engine torque" };
    bool changed = false;
    for (uint i = 0; i < names.length(); i++)
    {
        bool v = g_show[i];
        if (ImGui::Checkbox(names[i] + "##show" + i, v)) { g_show[i] = v; changed = true; }
    }
    if (ImGui::Checkbox("Outside temperature##showtemp", g_show_temp)) changed = true;
    LayoutSettings(changed);
    ImGui::Separator();
    if (ImGui::SliderFloat("Scale##scale", g_scale_mul, 0.6f, 2.0f)) changed = true;
    ImGui::Separator();
    ImGui::TextDisabled("Other dashboards: Settings > UI > Default airplane dashboard");
    if (changed)
        SaveSettings();
    ImGui::EndPopup();
}

// ---- Layout: every part is a separate element that can be moved ('Arrange' in the settings) ----

class Elem
{
    int id;
    float x, y, w, h;        // default place in design units
}

const float GRID = 10.f;     // design units
array<Elem@> g_elems;
array<float> g_off_x(EL_COUNT);   // user offset per element [design units]
array<float> g_off_y(EL_COUNT);
float g_grp_x = 0.f, g_grp_y = 0.f; // whole dashboard moved
bool  g_arrange = false;
bool  g_snap = true;
int   g_drag_el = -1;
float g_drag_x = 0.f, g_drag_y = 0.f; // unsnapped position while dragging
bool  g_save_pending = false;

void AddEl(int id, float x, float y, float w, float h)
{
    Elem e;
    e.id = id; e.x = x; e.y = y; e.w = w; e.h = h;
    g_elems.insertLast(e);
}

/// Background of the instruments/cards in the chosen colour and opacity
color BgCol(float lighten = 0.f)
{
    return color(fmin(1.f, g_col_bg.r + lighten), fmin(1.f, g_col_bg.g + lighten), fmin(1.f, g_col_bg.b + lighten), g_bg_pct / 100.f);
}

color BgBorder()
{
    return color(1.f, 1.f, 1.f, 0.14f * fmin(1.f, g_bg_pct / 50.f));
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

void LoadLayout()
{
    LoadColor("airplane_col_acc", g_col_acc);
    LoadColor("airplane_col_bg", g_col_bg);
    if (g_storage.exists("airplane_bg_pct"))
        g_bg_pct = int(clampf(float(g_storage.getInt("airplane_bg_pct")), 0.f, 100.f));
    else if (g_storage.exists("airplane_transparent") && g_storage.getBool("airplane_transparent"))
        g_bg_pct = 0; // older setting
    if (g_storage.exists("airplane_snap")) g_snap = g_storage.getBool("airplane_snap");
    if (g_storage.exists("airplane_grp_x")) { g_grp_x = g_storage.getFloat("airplane_grp_x"); g_grp_y = g_storage.getFloat("airplane_grp_y"); }
    for (int e = 0; e < EL_COUNT; e++)
    {
        const string key = "airplane_off" + e;
        if (g_storage.exists(key + "_x")) { g_off_x[e] = g_storage.getFloat(key + "_x"); g_off_y[e] = g_storage.getFloat(key + "_y"); }
    }
}

void SaveLayout()
{
    SaveColor("airplane_col_acc", g_col_acc);
    SaveColor("airplane_col_bg", g_col_bg);
    g_storage.setInt("airplane_bg_pct", g_bg_pct);
    g_storage.setBool("airplane_snap", g_snap);
    g_storage.setFloat("airplane_grp_x", g_grp_x);
    g_storage.setFloat("airplane_grp_y", g_grp_y);
    for (int e = 0; e < EL_COUNT; e++)
    {
        const string key = "airplane_off" + e;
        if (g_off_x[e] != 0.f || g_off_y[e] != 0.f || g_storage.exists(key + "_x"))
        {
            g_storage.setFloat(key + "_x", g_off_x[e]);
            g_storage.setFloat(key + "_y", g_off_y[e]);
        }
    }
}

/// Back to the default arrangement (place and order of the parts)
void ResetLayout()
{
    for (int e = 0; e < EL_COUNT; e++)
    {
        g_off_x[e] = 0.f;
        g_off_y[e] = 0.f;
    }
    g_grp_x = 0.f;
    g_grp_y = 0.f;
}

/// Draws all elements, each in its own window. 'base' = screen position of the layout origin.
void RunLayout(vector2 base, float k, bool element_bg)
{
    const vector2 scr = game.getDisplaySize();
    base = base + vector2(g_grp_x * k, g_grp_y * k);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, vector2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, color(0.f, 0.f, 0.f, 0.f));
    const int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;
    vector2 done_pos(scr.x * 0.5f, 8.f); // 'Done' button while arranging: above (or below) the settings element
    float done_bottom = 8.f;
    for (uint n = 0; n < g_elems.length(); n++)
    {
        Elem@ e = g_elems[n];
        const float pad = g_arrange ? 3.f : 0.f; // room for the frame
        float ox = g_off_x[e.id], oy = g_off_y[e.id];
        // keep it on the screen
        const float sx = base.x + (e.x + ox - pad) * k, sy = base.y + (e.y + oy - pad) * k;
        const float sw = (e.w + 2.f * pad) * k, sh = (e.h + 2.f * pad) * k;
        const float cx = clampf(sx, 0.f, fmax(0.f, scr.x - sw)), cy = clampf(sy, 0.f, fmax(0.f, scr.y - sh));
        ox += (cx - sx) / k; oy += (cy - sy) / k;
        if (e.id == EL_SETTINGS)
        {
            done_pos = vector2(cx + sw, cy);
            done_bottom = cy + sh;
        }
        ImGui::SetNextWindowPos(vector2(cx, cy), ImGuiCond_Always);
        ImGui::SetNextWindowSize(vector2(sw, sh));
        bool window_open = true;
        if (ImGui::Begin("AirDashboard##" + e.id, window_open, flags))
        {
            @c.dl = ImGui::GetWindowDrawList();
            c.o = vector2(base.x + ox * k, base.y + oy * k); // layout coordinates -> screen
            c.k = k;
            g_idc = 0;
            bool active;
            if (element_bg)
            {
                c.dl.AddRectFilled(c.P(e.x, e.y), c.P(e.x + e.w, e.y + e.h), BgCol(), 8.f * k);
                c.dl.AddRect(c.P(e.x, e.y), c.P(e.x + e.w, e.y + e.h), BgBorder(), 8.f * k, 15, 1.f);
            }
            if (g_arrange)
            {
                // the frame is submitted first, so it takes the clicks instead of the buttons
                LayoutArea(e.x - pad, e.y - pad, e.w + 2.f * pad, e.h + 2.f * pad, active);
                const bool hov = ImGui::IsItemHovered() || active;
                if (ImGui::IsItemClicked(1)) // right click ends arranging
                {
                    g_arrange = false;
                    g_drag_el = 99; // saves
                }
                DrawElement(e);
                DashedRect(e.x - pad + 1.f, e.y - pad + 1.f, e.w + 2.f * pad - 2.f, e.h + 2.f * pad - 2.f, hov ? COL_WHITE_FW : g_col_acc);
                if (active && ImGui::IsMouseDragging(0, 1.f))
                {
                    if (g_drag_el != e.id) { g_drag_el = e.id; g_drag_x = g_off_x[e.id]; g_drag_y = g_off_y[e.id]; }
                    const vector2 d = ImGui::GetMouseDragDelta(0, 1.f);
                    ImGui::ResetMouseDragDelta(0);
                    g_drag_x += d.x / k; g_drag_y += d.y / k;
                    g_off_x[e.id] = g_snap ? round_i(g_drag_x / GRID) * GRID : g_drag_x;
                    g_off_y[e.id] = g_snap ? round_i(g_drag_y / GRID) * GRID : g_drag_y;
                }
            }
            else
            {
                DrawElement(e);
                // dragging a free spot moves the whole dashboard (submitted last, so the buttons keep their clicks)
                LayoutArea(e.x, e.y, e.w, e.h, active);
                if (active && ImGui::IsMouseDragging(0, 1.f))
                {
                    const vector2 d = ImGui::GetMouseDragDelta(0, 1.f);
                    ImGui::ResetMouseDragDelta(0);
                    g_grp_x += d.x / k; g_grp_y += d.y / k;
                    g_drag_el = 99;
                }
            }
            if (e.id == EL_SETTINGS)
                DrawSettings();
        }
        ImGui::End();
    }
    if (g_arrange)
        DoneButton(done_pos, done_bottom, k);
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(2);
    if ((g_drag_el >= 0 || g_save_pending) && !ImGui::IsMouseDown(0))
    {
        g_drag_el = -1;
        g_save_pending = false;
        SaveSettings();
    }
}

bool LayoutArea(float x, float y, float w, float h, bool &out active)
{
    ImGui::SetCursorScreenPos(c.P(x, y));
    ImGui::PushID(g_idc++);
    ImGui::InvisibleButton("la", vector2(fmax(1.f, w * c.k), fmax(1.f, h * c.k)));
    ImGui::PopID();
    active = ImGui::IsItemActive();
    return ImGui::IsItemClicked(0);
}

const color COL_WHITE_FW = color(1.f, 1.f, 1.f, 1.f);

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

/// While arranging: 'Done' button (right aligned at pos, above it), ends the arrange mode
void DoneButton(const vector2&in pos, float bottom, float k)
{
    const string label = "Done";
    const float w = c.Size(label, 10.f).x / k + 24.f, h = 24.f;
    const float wx = clampf(pos.x - w * k, 0.f, game.getDisplaySize().x - w * k);
    float wy = pos.y - (h + 6.f) * k;
    if (wy < 0.f)
        wy = bottom + 6.f * k; // no room above: below the settings element
    ImGui::SetNextWindowPos(vector2(wx, wy), ImGuiCond_Always);
    ImGui::SetNextWindowSize(vector2(w * k, h * k));
    const int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
    bool window_open = true;
    if (ImGui::Begin("AirDashboard##done", window_open, flags))
    {
        @c.dl = ImGui::GetWindowDrawList();
        c.o = ImGui::GetWindowPos();
        c.k = k;
        g_idc = 0;
        bool active;
        const bool clicked = LayoutArea(0.f, 0.f, w, h, active);
        const bool hov = ImGui::IsItemHovered();
        c.dl.AddRectFilled(c.P(0.f, 0.f), c.P(w, h), hov ? COL_WHITE_FW : g_col_acc, 4.f * k);
        c.Text(w * 0.5f, h * 0.5f, label, 10.f, color(0.04f, 0.06f, 0.09f, 1.f));
        if (hov)
            ImGui::SetTooltip("End arranging (or right click a part)");
        if (clicked)
        {
            g_arrange = false;
            SaveSettings();
        }
    }
    ImGui::End();
}

/// Settings: colours, background, arrangement
void LayoutSettings(bool &inout changed)
{
    ImGui::Separator();
    ImGui::TextDisabled("Colours");
    ImGui::SetNextItemWidth(200.f);
    if (ImGui::ColorEdit3("Accent (needles)##colacc", g_col_acc)) g_save_pending = true; // saved after the mouse is released
    ImGui::SetNextItemWidth(200.f);
    if (ImGui::ColorEdit3("Background##colbg", g_col_bg)) g_save_pending = true;
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::SliderInt("Background opacity %##bgpct", g_bg_pct, 0, 100)) g_save_pending = true;
    if (ImGui::Button("Reset colours##colreset"))
    {
        g_col_acc = COL_ACC_DEF;
        g_col_bg = COL_BG_DEF;
        g_bg_pct = BG_PCT_DEF;
        changed = true;
    }
    ImGui::Separator();
    if (ImGui::Checkbox("Arrange##arrange", g_arrange)) changed = true;
    if (g_arrange)
    {
        ImGui::Indent(16.f);
        if (ImGui::Checkbox("Snap to grid##snap", g_snap)) changed = true;
        ImGui::TextDisabled("Drag the framed parts with the mouse; 'Done' or a right click ends it");
        ImGui::Unindent(16.f);
    }
    if (ImGui::Button("Reset arrangement##resetlayout"))
    {
        ResetLayout();
        changed = true;
    }
}

void Under(float cx, const string&in name, const string&in value)
{
    c.Text(cx, 177.f, name, 8.f, COL_MUTED);
    c.Text(cx, 191.f, value, 11.f, COL_WHITE);
}

// Point of the attitude indicator, rotated by the roll angle
float g_att_cx, g_att_cy, g_att_cr, g_att_sr;
vector2 Rot(float px, float py)
{
    return c.P(g_att_cx + px * g_att_cr - py * g_att_sr, g_att_cy + px * g_att_sr + py * g_att_cr);
}

void DrawAttitude(float cx, float cy, float R, const vector3&in dirv)
{
    const float k = c.k;
    const int pos_node = thisActor.getCameraPosNode(0);
    const int roll_node = thisActor.getCameraRollNode(0);
    vector3 rollv = thisActor.getNodePosition(pos_node) - thisActor.getNodePosition(roll_node);
    rollv.normalise();
    float roll = asin(clampf(rollv.y, -1.f, 1.f));
    const float pitch = asin(clampf(dirv.y, -1.f, 1.f));
    const vector3 upv = dirv.crossProduct(rollv * -1.f);
    if (upv.y < 0.f)
        roll = 3.14159f - roll;
    g_att_cx = cx; g_att_cy = cy; g_att_cr = cos(roll); g_att_sr = sin(roll);
    const float pitch_deg = pitch / DEG2RAD;
    const float ar = R - 8.f;
    const float ppd = 1.1f; // design units per degree of pitch
    const float d = clampf(pitch_deg * ppd, -ar * 1.5f, ar * 1.5f);
    c.dl.AddCircleFilled(c.P(cx, cy), R * k, DialBg(), 48);
    c.dl.AddCircleFilled(c.P(cx, cy), ar * k, COL_SKY, 48);
    if (d <= -ar)
    {
        c.dl.AddCircleFilled(c.P(cx, cy), ar * k, COL_GROUND, 48);
    }
    else if (d < ar)
    {
        const float phi = acos(d / ar);
        c.dl.PathClear();
        for (int i = 0; i <= 28; i++)
        {
            const float tt = phi - 2.f * phi * i / 28.f; // clockwise on screen
            c.dl.PathLineTo(Rot(ar * sin(tt), ar * cos(tt)));
        }
        c.dl.PathFillConvex(COL_GROUND);
    }
    // pitch ladder with degree numbers
    for (int deg = -40; deg <= 40; deg += 10)
    {
        const float y = d - deg * ppd;
        if (deg == 0 || abs(y) > ar - 7.f) continue;
        const bool major = (deg % 20 == 0);
        const float hw = major ? 12.f : 6.f;
        c.dl.AddLine(Rot(-hw, y), Rot(hw, y), COL_WHITE, 1.2f * k);
        if (major)
        {
            const string s = formatInt(int(abs(deg)));
            c.TextAt(Rot(-hw - 8.f, y), s, 8.f, COL_WHITE);
            c.TextAt(Rot(hw + 8.f, y), s, 8.f, COL_WHITE);
        }
    }
    if (abs(d) < ar)
        c.dl.AddLine(Rot(-ar, d), Rot(ar, d), COL_WHITE, 1.3f * k);
    // roll scale on the outer ring (fixed): 0, 30, 60
    c.dl.AddCircle(c.P(cx, cy), R * k, COL_RING, 48, 1.f);
    const array<float> roll_marks = { -60.f, -30.f, 0.f, 30.f, 60.f };
    for (uint i = 0; i < roll_marks.length(); i++)
    {
        c.dl.AddLine(c.Pol(cx, cy, R - 1.f, roll_marks[i]), c.Pol(cx, cy, R - 6.f, roll_marks[i]), COL_TICK, 1.3f * k);
        if (roll_marks[i] != 0.f)
            c.TextAt(c.Pol(cx, cy, R + 7.f, roll_marks[i]), formatInt(int(abs(roll_marks[i]))), 7.f, COL_MUTED);
    }
    // roll pointer (moves with the horizon) and fixed aircraft reference
    c.dl.AddTriangleFilled(Rot(0.f, -ar + 1.f), Rot(-3.f, -ar + 8.f), Rot(3.f, -ar + 8.f), COL_WHITE);
    c.dl.AddTriangleFilled(c.P(cx, cy - R + 1.f), c.P(cx - 3.f, cy - R - 5.f), c.P(cx + 3.f, cy - R - 5.f), g_col_acc);
    c.Line(cx - 36.f, cy, cx - 11.f, cy, g_col_acc, 3.f);
    c.Line(cx + 11.f, cy, cx + 36.f, cy, g_col_acc, 3.f);
    c.dl.AddCircleFilled(c.P(cx, cy), 3.f * k, g_col_acc, 10);
    float rd = roll / DEG2RAD;
    while (rd > 180.f) rd -= 360.f;
    while (rd < -180.f) rd += 360.f;
    Under(cx, "ATTITUDE", "P " + formatInt(int(pitch_deg), "+") + "  R " + formatInt(int(rd), "+"));
}

void DrawHsi(float cx, float cy, float R, const vector3&in dirv, AutopilotClass@ ap, int hdg_value)
{
    const float k = c.k;
    const float hdg = atan2(dirv.x, -dirv.z) / DEG2RAD;
    c.dl.AddCircleFilled(c.P(cx, cy), R * k, DialBg(), 48);
    c.dl.AddCircle(c.P(cx, cy), R * k, COL_RING, 48, 1.f);
    const array<string> names = { "N", "3", "6", "E", "12", "15", "S", "21", "24", "W", "30", "33" };
    for (int i = 0; i < 12; i++)
    {
        const float a = i * 30.f - hdg;
        c.dl.AddLine(c.Pol(cx, cy, R - 1.5f, a), c.Pol(cx, cy, R - 7.f, a), COL_TICK, 1.5f * k);
        c.TextAt(c.Pol(cx, cy, R - 15.f, a), names[i], 10.f, (i % 3 == 0) ? g_col_acc : COL_TEXT);
    }
    const float ba = float(hdg_value) - hdg;
    c.dl.AddTriangleFilled(c.Pol(cx, cy, R - 1.f, ba), c.Pol(cx, cy, R - 9.f, ba - 6.f), c.Pol(cx, cy, R - 9.f, ba + 6.f), COL_BLUE);
    c.dl.AddTriangleFilled(c.P(cx, cy - 18.f), c.P(cx - 5.f, cy - 6.f), c.P(cx + 5.f, cy - 6.f), g_col_acc);
    c.Line(cx, cy - 6.f, cx, cy + 18.f, COL_TICK, 2.f);
    if (ap !is null && ap.isILSAvailable())
    {
        const float hd = clampf(ap.getHorizontalApproachDeviation(), -15.f, 15.f) / 15.f * 28.f;
        const float vd = clampf(ap.getVerticalApproachDeviation(), -15.f, 15.f) / 15.f * 28.f;
        c.Line(cx + hd, cy - 28.f, cx + hd, cy + 28.f, COL_GREEN, 1.5f);
        c.Line(cx - 28.f, cy + vd, cx + 28.f, cy + vd, COL_GREEN, 1.5f);
    }
    float hd360 = hdg + 360.f;
    hd360 -= 360.f * floor(hd360 / 360.f);
    Under(cx, "HEADING", Pad3(int(hd360)));
}

void DrawEngine(int i, float x0, bool has_dials)
{
    const float k = c.k;
    AircraftEngineClass@ ae = thisActor.getAircraftEngine(i);
    if (ae is null)
        return;
    TurbopropClass@ tp = thisActor.getTurboprop(i);
    const bool is_prop = (tp !is null);
    const float ex = x0 + 22.f;
    const array<float> t_rpm = { 0, 60, 110 };
    const array<string> l_rpm = { "0", "60", "110" };
    int row = 0;
    if (g_show[6])
    {
        const float ey = 30.f + 58.f * row++;
        const float rpm = ae.getRPMPercent();
        Dial(ex, ey, 22.f, false, AngleRpm, t_rpm, l_rpm, rpm, 7.f);
        c.TextLR(ex, ey + 32.f, "RPM ", formatInt(int(rpm)) + "%", 8.f);
    }
    if (is_prop && g_show[7])
    {
        const float ey = 30.f + 58.f * row++;
        const array<float> t_p = { -60, 0, 60 };
        const array<string> l_p = { "-", "0", "+" };
        const float pitch = tp.getPropellerPitch();
        Dial(ex, ey, 22.f, false, AnglePitch, t_p, l_p, pitch, 7.f);
        c.TextLR(ex, ey + 32.f, "PITCH ", formatInt(int(pitch)), 8.f);
    }
    if (is_prop && g_show[8])
    {
        const float ey = 30.f + 58.f * row++;
        const float max_torque = tp.getPropellerMaxTorque();
        const float torque = (max_torque != 0.f) ? 100.f * tp.getPropellerIndicatedTorque() / max_torque : 0.f;
        Dial(ex, ey, 22.f, false, AngleRpm, t_rpm, l_rpm, torque, 7.f);
        c.TextLR(ex, ey + 32.f, "TORQ ", formatInt(int(torque)) + "%", 8.f);
    }
    // Throttle lever
    const float tx = x0 + (has_dials ? 50.f : 14.f), ty = 12.f, th = 160.f;
    c.dl.AddRectFilled(c.P(tx, ty), c.P(tx + 8.f, ty + th), COL_PILL, 4.f * k);
    const float thr = clampf(ae.getThrottle(), 0.f, 1.f);
    const float hy = ty + (1.f - thr) * (th - 8.f);
    c.dl.AddRectFilled(c.P(tx - 3.f, hy), c.P(tx + 11.f, hy + 8.f), g_col_acc, 2.f * k);
    ImGui::SetCursorScreenPos(c.P(tx - 6.f, ty - 4.f));
    ImGui::PushID(1000 + i);
    ImGui::InvisibleButton("thr", vector2(20.f * k, (th + 8.f) * k));
    ImGui::PopID();
    if (ImGui::IsItemActive())
    {
        const float v = 1.f - ((ImGui::GetMousePos().y - c.P(0.f, ty).y - 4.f * k) / ((th - 8.f) * k));
        ae.setThrottle(clampf(v, 0.f, 1.f));
    }
    // Start button, centered under the lever (turns red with FIRE on engine failure)
    const float px0 = tx + 4.f - 14.f;
    const bool failed = ae.isFailed();
    const bool ignition = ae.getIgnition();
    c.dl.AddRectFilled(c.P(px0, 183.f), c.P(px0 + 28.f, 195.f), failed ? COL_RED : (ignition ? COL_GREEN : COL_PILL), 6.f * k);
    c.Text(px0 + 14.f, 189.f, failed ? "FIRE" : (ignition ? "ON" : "OFF"), 8.f, COL_WHITE);
    if (Hit(px0, 183.f, 28.f, 12.f))
        ae.flipStart();
}

void main()
{
    LoadSettings();
}
