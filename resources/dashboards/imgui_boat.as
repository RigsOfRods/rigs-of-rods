// Built-in boat dashboard of Rigs of Rods, drawn with DearIMGUI from AngelScript.
// Loaded by 'imgui_boat.dashboard' (a dashboard without '.layout' file), see DashBoardManager::loadDashBoard().
// Speed, one throttle lever per screwprop (with engine switch), heading + rudder (with 'center rudder' button),
// echo sounder with history, heel/trim and light switches.
// ---------------------------------------------------------------------------------------------------------------

const float PI_F = 3.14159265f;
const float DEG2RAD = PI_F / 180.f;
const float MS_TO_KNOTS = 1.9438f;
const float MS_TO_KMH = 3.6f;
const int   DEPTH_SAMPLES = 40;        // Echo sounder history length
const float DEPTH_INTERVAL = 0.5f;     // Seconds between echo sounder readings
const float DEPTH_ALARM = 2.f;         // Shallow water warning [m]
const int   MAX_LIGHT_SWITCHES = 10;   // custom lights 1..10

color Col(int r, int g, int b, int a = 255) { return color(r / 255.f, g / 255.f, b / 255.f, a / 255.f); }

const color COL_CARD_BR = Col(42, 49, 58);
const color COL_INSET   = Col(11, 16, 22);
const color COL_TEXT    = Col(230, 237, 243);
const color COL_MUTED   = Col(157, 167, 179);
const color COL_LINE    = Col(58, 68, 80);
const color COL_ACC_DEF = Col(63, 184, 232);   // default accent
const color COL_BG_DEF  = Col(22, 27, 34);     // default card background
const int   BG_PCT_DEF  = 92;                  // default background opacity [%]
const color COL_WARN    = Col(240, 160, 32);
const color COL_PORT    = Col(229, 83, 75);   // red = port (backboard)
const color COL_STBD    = Col(63, 185, 80);   // green = starboard
const color COL_BAR     = Col(47, 111, 143);
const color COL_BTN     = Col(33, 38, 45);
const color COL_BTN_HOV = Col(48, 55, 64);

float fmin(float a, float b) { return (a < b) ? a : b; }
float fmax(float a, float b) { return (a > b) ? a : b; }
float clampf(float v, float lo, float hi) { return fmin(fmax(v, lo), hi); }
int   round_i(float v) { return int(floor(v + 0.5f)); }

// ---- Settings (kept in 'imgui_dashboards.asdata') ----

float g_scale_mul = 1.f;
color g_col_acc = COL_ACC_DEF;
color g_col_bg = COL_BG_DEF;
int   g_bg_pct = BG_PCT_DEF;
array<bool> g_show = { true, true, true, true, true }; // speed, throttle, heading & rudder, echo sounder, attitude & switches
LocalStorageClass@ g_storage;

void LoadSettings()
{
    @g_storage = LocalStorageClass("imgui_dashboards");
    for (uint i = 0; i < g_show.length(); i++)
    {
        if (g_storage.exists("boat_show" + i))
            g_show[i] = g_storage.getBool("boat_show" + i);
    }
    LoadLayout();
    if (g_storage.exists("boat_scale"))       g_scale_mul = clampf(g_storage.getFloat("boat_scale"), 0.5f, 1.6f);
}

void SaveSettings()
{
    for (uint i = 0; i < g_show.length(); i++)
        g_storage.setBool("boat_show" + i, g_show[i]);
    SaveLayout();
    g_storage.setFloat("boat_scale", g_scale_mul);
    g_storage.save();
}

// ---- State ----

float g_speed_max = 20.f;
float g_depth = -1.f;
float g_depth_timer = 0.f;
array<float> g_depth_history;
int   g_idc = 0;

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
    void Card(float x, float w, const string&in title)
    {
        dl.AddRectFilled(P(x, 20.f), P(x + w, 160.f), BgCol(), 6.f * k);
        dl.AddRect(P(x, 20.f), P(x + w, 160.f), BgBorder(), 6.f * k, 15, 1.f);
        TextL(x + 7.f, 28.f, title, 7.5f, COL_MUTED);
    }
    /// Arc stroke, angles in degrees clockwise from top
    void Arc(float cx, float cy, float r, float a0, float a1, const color&in col, float w)
    {
        // ImGui: radians, 0 = +x (right), clockwise on screen (y down)
        dl.PathClear();
        dl.PathArcTo(P(cx, cy), r * k, (a0 - 90.f) * DEG2RAD, (a1 - 90.f) * DEG2RAD, 32);
        dl.PathStroke(col, false, w * k);
    }
}

Ctx c;

string CompassPoint(int idx)
{
    const array<string> names = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    return names[((idx % 8) + 8) % 8];
}

string HeadingName(float hdg)
{
    return CompassPoint(int(floor((hdg + 22.5f) / 45.f)));
}

const string PORT = "P";
const string STBD = "S";

// Clickable area; 'active' = mouse button held on it
bool Area(float x, float y, float w, float h, bool &out active)
{
    ImGui::SetCursorScreenPos(c.P(x, y));
    ImGui::PushID(g_idc++);
    ImGui::InvisibleButton("a", vector2(w * c.k, h * c.k));
    ImGui::PopID();
    active = ImGui::IsItemActive();
    return ImGui::IsItemClicked(0);
}

bool Button(float x, float y, float w, float h, const string&in label, bool on, const string&in tooltip)
{
    bool active;
    const bool clicked = Area(x, y, w, h, active);
    const bool hovered = ImGui::IsItemHovered();
    c.dl.AddRectFilled(c.P(x, y), c.P(x + w, y + h), on ? g_col_acc : (hovered ? COL_BTN_HOV : COL_BTN), 4.f * c.k);
    c.dl.AddRect(c.P(x, y), c.P(x + w, y + h), on ? g_col_acc : COL_CARD_BR, 4.f * c.k, 15, 1.f);
    c.Text(x + w * 0.5f, y + h * 0.5f, label, 7.5f, on ? COL_INSET : COL_TEXT);
    if (hovered && tooltip != "")
        ImGui::SetTooltip(tooltip);
    return clicked;
}

// ---- Main ----

void frameStep(float dt)
{
    if (thisActor is null || g_storage is null)
        return;
    BeamClass@ player = game.getCurrentTruck();
    if (player is null || player.getInstanceId() != thisActor.getInstanceId())
        return;
    DashBoardManagerClass@ dashboard = thisActor.getDashboardManager();
    if (dashboard is null || !dashboard.getVisible())
        return;
    const int n_props = thisActor.getScrewpropCount();
    const int num_nodes = thisActor.getNodeCount();
    if (n_props == 0 || num_nodes == 0)
        return;

    // ---- Readings ----
    const vector3 dir = thisActor.getDirection();
    const float speed_ms = dir.dotProduct(thisActor.getNodeVelocity(0));
    const float knots = speed_ms * MS_TO_KNOTS;
    while (abs(knots) > g_speed_max * 0.9f && g_speed_max < 80.f)
        g_speed_max += 20.f;

    float heading = atan2(dir.x, -dir.z) / DEG2RAD; // 0 = north (-Z), clockwise
    if (heading < 0.f)
        heading += 360.f;

    // Heel and trim from the camera reference nodes: forward = direction, side = pos -> roll node
    float heel = 0.f;
    const float trim = asin(clampf(dir.y, -1.f, 1.f)) / DEG2RAD; // > 0 = bow up
    {
        const int pos_node = thisActor.getCameraPosNode(0);
        const int roll_node = thisActor.getCameraRollNode(0);
        if (pos_node >= 0 && roll_node >= 0 && pos_node < num_nodes && roll_node < num_nodes && pos_node != roll_node)
        {
            vector3 side = thisActor.getNodePosition(pos_node) - thisActor.getNodePosition(roll_node);
            side.normalise();
            vector3 stbd = dir.crossProduct(vector3(0.f, 1.f, 0.f));
            stbd.normalise();
            if (side.dotProduct(stbd) < 0.f)
                side = side * -1.f; // make it point to starboard
            heel = -asin(clampf(side.y, -1.f, 1.f)) / DEG2RAD; // > 0 = heeling to starboard
        }
    }

    // Echo sounder: distance of the lowest hull point to the ground below it, a few times per second
    g_depth_timer -= dt;
    if (g_depth_timer <= 0.f)
    {
        g_depth_timer = DEPTH_INTERVAL;
        float agl = FLT_MAX;
        for (int i = 0; i < num_nodes; i++)
        {
            vector3 p = thisActor.getNodePosition(i);
            agl = fmin(agl, p.y - game.getGroundHeight(p));
        }
        g_depth = (agl > 0.05f && agl < 999.f) ? agl : -1.f;
        g_depth_history.insertLast(fmax(0.f, g_depth));
        if (int(g_depth_history.length()) > DEPTH_SAMPLES)
            g_depth_history.removeAt(0);
    }

    ScrewpropClass@ prop0 = thisActor.getScrewprop(0);
    const float rudder = (prop0 !is null) ? prop0.getRudder() : 0.f; // -1..1, > 0 = port (keyboard 'left')

    g_knots = knots; g_speed_ms = speed_ms; g_heading = heading; g_rudder = rudder; g_heel = heel; g_trim = trim; g_n_props = n_props;

    // ---- Layout (design units) ----
    const float GAP = 6.f;
    float x = 6.f;
    g_elems.resize(0);
    AddEl(EL_HEADER, 4.f, 0.f, c.Size(thisActor.getTruckName(), 7.5f).x / fmax(c.k, 0.01f) + 34.f, 18.f);
    const array<float> widths = { 116.f, fmax(2.f, float(n_props)) * 30.f + 46.f, 170.f, 140.f, 150.f };
    for (int i = 0; i < 5; i++)
    {
        if (!g_show[i])
            continue;
        AddEl(EL_SPEED + i, x, 20.f, widths[i], 140.f);
        x += widths[i] + GAP;
    }
    const float DW = fmax(x, 200.f);
    const float DH = 166.f;

    const vector2 scr = game.getDisplaySize();
    const float k = fmax(0.5f, fmin((scr.x * 0.5f / 640.f) * g_scale_mul, scr.x * 0.97f / DW));
    c.k = k;
    g_elems[0].w = c.Size(thisActor.getTruckName(), 7.5f).x / k + 34.f; // header: name + settings button
    RunLayout(vector2((scr.x - DW * k) * 0.5f, scr.y - DH * k - 8.f), k, false);
}

// ---- Elements ----

const int EL_HEADER = 0;
const int EL_SPEED = 1;
const int EL_LEVER = 2;
const int EL_COURSE = 3;
const int EL_DEPTH = 4;
const int EL_ATT = 5;
const int EL_COUNT = 6;
const int EL_SETTINGS = EL_HEADER; // element with the settings button (and popup)

float g_knots = 0.f, g_speed_ms = 0.f, g_heading = 0.f, g_rudder = 0.f, g_heel = 0.f, g_trim = 0.f;
int   g_n_props = 0;

void DrawElement(const Elem@ e)
{
    switch (e.id)
    {
    case EL_HEADER: DrawHeader(e.x, e.w); break;
    case EL_SPEED: DrawSpeed(e.x, e.w, g_knots, g_speed_ms); break;
    case EL_LEVER: DrawLevers(e.x, e.w, g_n_props); break;
    case EL_COURSE: DrawCourse(e.x, e.w, g_heading, g_rudder, g_n_props); break;
    case EL_DEPTH: DrawDepth(e.x, e.w); break;
    case EL_ATT: DrawAttitude(e.x, e.w, g_heel, g_trim); break;
    }
}

/// Vehicle name and settings button
void DrawHeader(float x, float w)
{
    const float k = c.k;
    bool active;
    c.dl.AddRectFilled(c.P(x, 0.f), c.P(x + w, 18.f), BgCol(), 6.f * k);
    c.TextL(x + 6.f, 9.f, thisActor.getTruckName(), 7.5f, COL_TEXT);
    if (Area(x + w - 18.f, 1.f, 16.f, 16.f, active))
        ImGui::OpenPopup("BoatDashboardSettings");
    const bool hov = ImGui::IsItemHovered();
    const vector2 gc = c.P(x + w - 10.f, 9.f);
    c.dl.AddCircle(gc, 3.6f * k, hov ? COL_TEXT : COL_MUTED, 12, 1.4f * k);
    for (int i = 0; i < 8; i++)
    {
        const float a = i * 45.f;
        c.dl.AddLine(c.Pol(x + w - 10.f, 9.f, 4.2f, a), c.Pol(x + w - 10.f, 9.f, 6.f, a), hov ? COL_TEXT : COL_MUTED, 1.6f * k);
    }
    if (hov)
        ImGui::SetTooltip("Dashboard settings");
}

void DrawSettings()
{
    if (!ImGui::BeginPopup("BoatDashboardSettings"))
        return;
    ImGui::TextDisabled("Boat dashboard");
    bool changed = false;
    const array<string> names = { "Speed", "Throttle", "Heading & rudder", "Echo sounder", "Attitude & switches" };
    for (uint i = 0; i < names.length(); i++)
    {
        bool v = g_show[i];
        if (ImGui::Checkbox(names[i] + "##show" + i, v)) { g_show[i] = v; changed = true; }
    }
    LayoutSettings(changed);
    ImGui::Separator();
    ImGui::SetNextItemWidth(160.f);
    if (ImGui::SliderFloat("Size", g_scale_mul, 0.5f, 1.6f)) changed = true;
    ImGui::Separator();
    ImGui::TextDisabled("Other dashboards: Settings > UI > Default boat dashboard");
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
    LoadColor("boat_col_acc", g_col_acc);
    LoadColor("boat_col_bg", g_col_bg);
    if (g_storage.exists("boat_bg_pct"))
        g_bg_pct = int(clampf(float(g_storage.getInt("boat_bg_pct")), 0.f, 100.f));
    else if (g_storage.exists("boat_transparent") && g_storage.getBool("boat_transparent"))
        g_bg_pct = 0; // older setting
    if (g_storage.exists("boat_snap")) g_snap = g_storage.getBool("boat_snap");
    if (g_storage.exists("boat_grp_x")) { g_grp_x = g_storage.getFloat("boat_grp_x"); g_grp_y = g_storage.getFloat("boat_grp_y"); }
    for (int e = 0; e < EL_COUNT; e++)
    {
        const string key = "boat_off" + e;
        if (g_storage.exists(key + "_x")) { g_off_x[e] = g_storage.getFloat(key + "_x"); g_off_y[e] = g_storage.getFloat(key + "_y"); }
    }
}

void SaveLayout()
{
    SaveColor("boat_col_acc", g_col_acc);
    SaveColor("boat_col_bg", g_col_bg);
    g_storage.setInt("boat_bg_pct", g_bg_pct);
    g_storage.setBool("boat_snap", g_snap);
    g_storage.setFloat("boat_grp_x", g_grp_x);
    g_storage.setFloat("boat_grp_y", g_grp_y);
    for (int e = 0; e < EL_COUNT; e++)
    {
        const string key = "boat_off" + e;
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
        if (ImGui::Begin("BoatDashboard##" + e.id, window_open, flags))
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
    if (ImGui::Begin("BoatDashboard##done", window_open, flags))
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
    if (ImGui::ColorEdit3("Accent (speed, levers)##colacc", g_col_acc)) g_save_pending = true; // saved after the mouse is released
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

void DrawSpeed(float x_speed, float W_SPEED, float knots, float speed_ms)
{
    const float k = c.k;
    c.Card(x_speed, W_SPEED, "Speed");
    const float cx = x_speed + W_SPEED * 0.5f, cy = 92.f, r = 44.f;
    c.Arc(cx, cy, r, -135.f, 135.f, COL_LINE, 3.f);
    if (knots > 0.f)
        c.Arc(cx, cy, r, -135.f, SpeedAngle(knots), g_col_acc, 3.f);
    const int steps = 8;
    for (int i = 0; i <= steps; i++)
    {
        const float v = g_speed_max * i / steps;
        const float a = SpeedAngle(v);
        c.dl.AddLine(c.Pol(cx, cy, r - 7.f, a), c.Pol(cx, cy, r - 2.f, a), COL_MUTED, 1.2f * k);
        if (i % 2 == 0)
            c.TextAt(c.Pol(cx, cy, r - 14.f, a), formatInt(int(v)), 7.f, COL_MUTED);
    }
    const float na = SpeedAngle(abs(knots));
    c.dl.AddLine(c.P(cx, cy), c.Pol(cx, cy, r - 9.f, na), COL_TEXT, 2.f * k);
    c.dl.AddCircleFilled(c.P(cx, cy), 3.f * k, COL_TEXT, 12);
    c.Text(cx, cy + 22.f, formatFloat(knots, "", 0, 1), 13.f, COL_TEXT);
    c.Text(cx, cy + 36.f, "kn \u00b7 " + formatFloat(speed_ms * MS_TO_KMH, "", 0, 0) + " km/h", 7.f, COL_MUTED);
}

float SpeedAngle(float v)
{
    return -135.f + clampf(v, 0.f, g_speed_max) / g_speed_max * 270.f;
}

// Throttle levers, one per screwprop (drag; Ctrl = all levers; double click = stop)
void DrawLevers(float x_lever, float W_LEVER, int n_props)
{
    const float k = c.k;
    c.Card(x_lever, W_LEVER, "Throttle");
    const float top = 44.f, bot = 120.f;
    const float x0 = x_lever + 40.f;
    // scale labels
    c.TextR(x0 - 6.f, LeverY(1.f, top, bot), "Ahead", 6.5f, COL_MUTED);
    c.TextR(x0 - 6.f, LeverY(0.f, top, bot), "Stop", 6.5f, COL_TEXT);
    c.TextR(x0 - 6.f, LeverY(-1.f, top, bot), "Astern", 6.5f, COL_MUTED);
    const array<float> marks = { 1.f, 0.5f, 0.f, -0.5f, -1.f };
    for (int i = 0; i < n_props; i++)
    {
        ScrewpropClass@ sp = thisActor.getScrewprop(i);
        if (sp is null)
            continue;
        const float lx = x0 + i * 30.f + 8.f;
        const float v = sp.getThrottle();
        for (uint m = 0; m < marks.length(); m++)
            c.Line(lx - 9.f, LeverY(marks[m], top, bot), lx + 9.f, LeverY(marks[m], top, bot), marks[m] == 0.f ? COL_TEXT : COL_LINE, marks[m] == 0.f ? 1.4f : 1.f);
        c.dl.AddRectFilled(c.P(lx - 3.f, top), c.P(lx + 3.f, bot), COL_INSET, 3.f * k);
        c.dl.AddRectFilled(c.P(lx - 3.f, fmin(LeverY(v, top, bot), LeverY(0.f, top, bot))),
                           c.P(lx + 3.f, fmax(LeverY(v, top, bot), LeverY(0.f, top, bot))), v >= 0.f ? g_col_acc : COL_WARN, 2.f * k);
        c.dl.AddRectFilled(c.P(lx - 10.f, LeverY(v, top, bot) - 4.f), c.P(lx + 10.f, LeverY(v, top, bot) + 4.f), COL_TEXT, 2.f * k);

        bool active;
        Area(lx - 12.f, top - 6.f, 24.f, bot - top + 12.f, active);
        const bool hovered = ImGui::IsItemHovered();
        if (hovered && ImGui::IsMouseDoubleClicked(0))
        {
            sp.setThrottle(0.f);
        }
        else if (active)
        {
            float nv = 1.f - 2.f * (ImGui::GetMousePos().y - c.P(0.f, top).y) / ((bot - top) * k);
            nv = clampf(nv, -1.f, 1.f);
            if (abs(nv) < 0.04f)
                nv = 0.f; // snap to stop
            if (inputs.isKeyDown(KC_LCTRL) || inputs.isKeyDown(KC_RCTRL))
            {
                for (int j = 0; j < n_props; j++)
                {
                    ScrewpropClass@ spj = thisActor.getScrewprop(j);
                    if (spj !is null)
                        spj.setThrottle(nv);
                }
            }
            else
            {
                sp.setThrottle(nv);
            }
        }
        if (hovered && !active)
            ImGui::SetTooltip("Drag to set the throttle, Ctrl: all levers, double click: stop");

        const string name = (n_props == 2) ? ((i == 0) ? PORT : STBD) : formatInt(i + 1);
        c.Text(lx, 128.f, name + " " + formatInt(round_i(v * 100.f)) + "%", 6.5f, COL_TEXT);

        // Engine switch (like the airplane dashboard's start buttons)
        const bool on = sp.isEngineOn();
        if (Button(lx - 13.f, 136.f, 26.f, 16.f, on ? "On" : "Off", on,
                   on ? "Engine running - click to switch off" : "Engine off - click to start"))
        {
            sp.setEngineOn(!on);
        }
    }
}

float LeverY(float v, float top, float bot)
{
    return top + (1.f - (v + 1.f) * 0.5f) * (bot - top);
}

void DrawCourse(float x_course, float W_COURSE, float heading, float rudder, int n_props)
{
    const float k = c.k;
    c.Card(x_course, W_COURSE, "Heading & rudder");
    c.TextL(x_course + 8.f, 44.f, formatInt(round_i(heading) % 360, "0", 3) + "\u00b0", 15.f, COL_TEXT);
    c.TextL(x_course + 60.f, 46.f, HeadingName(heading), 8.f, COL_MUTED);

    // compass tape
    const float tx0 = x_course + 8.f, tx1 = x_course + W_COURSE - 8.f, ty0 = 56.f, ty1 = 76.f;
    const float tcx = (tx0 + tx1) * 0.5f, ppd = 2.4f; // units per degree
    c.dl.AddRectFilled(c.P(tx0, ty0), c.P(tx1, ty1), COL_INSET, 4.f * k);
    ImGui::PushClipRect(c.P(tx0, ty0), c.P(tx1, ty1), true);
    const int first = int(floor(heading - 35.f));
    for (int d = first; d <= first + 70; d++)
    {
        const float x = tcx + (d - heading) * ppd;
        const int dd = ((d % 360) + 360) % 360;
        if (dd % 10 == 0)
        {
            c.Line(x, ty1 - 6.f, x, ty1, COL_MUTED, 1.2f);
            if (dd % 90 == 0)
                c.Text(x, ty0 + 6.f, CompassPoint(dd / 45), 8.f, COL_TEXT);
            else if (dd % 30 == 0)
                c.Text(x, ty0 + 6.f, formatInt(dd, "0", 3), 6.5f, COL_MUTED);
        }
        else if (dd % 5 == 0)
        {
            c.Line(x, ty1 - 3.f, x, ty1, COL_LINE, 1.f);
        }
    }
    ImGui::PopClipRect();
    c.dl.AddTriangleFilled(c.P(tcx - 4.f, ty1), c.P(tcx + 4.f, ty1), c.P(tcx, ty1 - 6.f), g_col_acc);

    // rudder half dial, port left / starboard right
    const float rcx = x_course + 66.f, rcy = 152.f, rr = 58.f;
    c.Arc(rcx, rcy, rr, -90.f, 0.f, COL_PORT, 4.f);
    c.Arc(rcx, rcy, rr, 0.f, 90.f, COL_STBD, 4.f);
    for (int a = -90; a <= 90; a += 15) // dial +-90 = rudder +-45
    {
        const bool major = (a % 30 == 0);
        c.dl.AddLine(c.Pol(rcx, rcy, rr - (major ? 8.f : 5.f), float(a)), c.Pol(rcx, rcy, rr - 1.f, float(a)), COL_MUTED, (major ? 1.4f : 1.f) * k);
        if (major)
            c.TextAt(c.Pol(rcx, rcy, rr - 17.f, float(a)), formatInt(((a < 0) ? -a : a) / 2), 7.5f, COL_MUTED);
    }
    const float rdeg = rudder * 45.f; // 'rudder' in degrees, > 0 = port
    c.dl.AddLine(c.P(rcx, rcy), c.Pol(rcx, rcy, rr - 8.f, -rdeg * 2.f), COL_TEXT, 2.5f * k);
    c.dl.AddCircleFilled(c.P(rcx, rcy), 4.f * k, COL_TEXT, 12);
    c.TextL(rcx - rr + 4.f, rcy - 6.f, PORT, 7.5f, COL_PORT);
    c.TextR(rcx + rr - 4.f, rcy - 6.f, STBD, 7.5f, COL_STBD);

    const string rval = (abs(rdeg) < 0.5f)
        ? "0\u00b0"
        : formatFloat(abs(rdeg), "", 0, 0) + "\u00b0 " + ((rdeg > 0.f) ? PORT : STBD);
    c.TextL(x_course + 134.f, 100.f, "Rudder", 7.5f, COL_MUTED);
    c.TextL(x_course + 134.f, 112.f, rval, 9.f, COL_TEXT);
    if (Button(x_course + 134.f, 128.f, 28.f, 18.f, "0\u00b0", false, "Center the rudder"))
    {
        for (int j = 0; j < n_props; j++)
        {
            ScrewpropClass@ sp = thisActor.getScrewprop(j);
            if (sp !is null)
                sp.setRudder(0.f);
        }
    }
}

void DrawDepth(float x_depth, float W_DEPTH)
{
    const float k = c.k;
    c.Card(x_depth, W_DEPTH, "Echo sounder");
    const bool shallow = (g_depth >= 0.f && g_depth < DEPTH_ALARM * 2.f);
    c.TextL(x_depth + 8.f, 44.f, (g_depth >= 0.f) ? formatFloat(g_depth, "", 0, 1) + " m" : "--.- m", 15.f, shallow ? COL_WARN : COL_TEXT);
    c.TextR(x_depth + W_DEPTH - 8.f, 46.f, "below keel", 6.5f, COL_MUTED);

    const float gx0 = x_depth + 8.f, gx1 = x_depth + W_DEPTH - 8.f, gy0 = 56.f, gy1 = 146.f;
    c.dl.AddRectFilled(c.P(gx0, gy0), c.P(gx1, gy1), COL_INSET, 4.f * k);
    float dmax = 10.f;
    for (uint i = 0; i < g_depth_history.length(); i++)
        dmax = fmax(dmax, g_depth_history[i] * 1.2f);
    const float bw = (gx1 - gx0) / DEPTH_SAMPLES;
    const int offset = DEPTH_SAMPLES - int(g_depth_history.length());
    for (uint i = 0; i < g_depth_history.length(); i++)
    {
        const float d = g_depth_history[i];
        const float bx = gx0 + (offset + int(i)) * bw;
        const float h = fmin(d / dmax, 1.f) * (gy1 - gy0 - 4.f);
        c.dl.AddRectFilled(c.P(bx + 0.4f, gy0 + 2.f), c.P(bx + bw - 0.4f, gy0 + 2.f + h), (d < DEPTH_ALARM * 2.f) ? COL_WARN : COL_BAR, 1.f * k);
    }
    const float ya = gy0 + 2.f + DEPTH_ALARM / dmax * (gy1 - gy0 - 4.f);
    c.Line(gx0, ya, gx1, ya, COL_PORT, 1.f);
    c.TextR(gx1 - 2.f, ya + 5.f, formatFloat(DEPTH_ALARM, "", 0, 0) + " m", 6.f, COL_PORT);
    c.TextL(x_depth + 8.f, 154.f, "last " + formatInt(int(DEPTH_SAMPLES * DEPTH_INTERVAL)) + " s", 6.5f, COL_MUTED);
}

void DrawAttitude(float x_att, float W_ATT, float heel, float trim)
{
    const float k = c.k;
    c.Card(x_att, W_ATT, "Attitude & switches");
    const float icx = x_att + 40.f, icy = 96.f, ir = 36.f;
    c.Arc(icx, icy, ir, -60.f, 60.f, COL_LINE, 4.f); // upper arc, the marker leans to the side the boat heels to
    for (int a = -30; a <= 30; a += 15)
        c.dl.AddLine(c.Pol(icx, icy, ir - 5.f, float(a)), c.Pol(icx, icy, ir + 3.f, float(a)), (a == 0) ? COL_TEXT : COL_MUTED, 1.3f * k);
    c.TextAt(c.Pol(icx, icy, ir + 10.f, -30.f), "30", 6.f, COL_MUTED);
    c.TextAt(c.Pol(icx, icy, ir + 10.f, 30.f), "30", 6.f, COL_MUTED);
    const float hb = clampf(heel, -55.f, 55.f);
    c.dl.AddCircleFilled(c.Pol(icx, icy, ir, hb), 5.f * k, (abs(heel) > 20.f) ? COL_WARN : g_col_acc, 12);

    const string heel_s = (abs(heel) < 0.5f)
        ? "0\u00b0"
        : formatFloat(abs(heel), "", 0, 0) + "\u00b0 " + ((heel > 0.f) ? STBD : PORT);
    const string trim_s = formatFloat(trim, "+", 0, 0) + "\u00b0";
    c.TextL(x_att + 86.f, 62.f, "Heel", 7.5f, COL_MUTED);
    c.TextL(x_att + 86.f, 74.f, heel_s, 11.f, COL_TEXT);
    c.TextL(x_att + 86.f, 90.f, "Trim", 7.5f, COL_MUTED);
    c.TextL(x_att + 86.f, 102.f, trim_s, 11.f, COL_TEXT);

    // Switches: navigation lights (head/side lights), beacons, custom lights 1..n
    array<string> sw_label;
    array<bool> sw_on;
    array<int> sw_kind; // 0 lights, 1 beacons, 2 custom
    array<int> sw_n;
    if (thisActor.countFlaresByType(FLARE_TYPE_HEADLIGHT) + thisActor.countFlaresByType(FLARE_TYPE_SIDELIGHT) > 0)
    {
        sw_label.insertLast("Lights"); sw_on.insertLast(thisActor.getHeadlightsVisible()); sw_kind.insertLast(0); sw_n.insertLast(0);
    }
    if (thisActor.countBeaconProps() > 0)
    {
        sw_label.insertLast("Beacons"); sw_on.insertLast(thisActor.getBeaconMode()); sw_kind.insertLast(1); sw_n.insertLast(0);
    }
    for (int n = 0; n < MAX_LIGHT_SWITCHES && sw_label.length() < 4; n++)
    {
        if (thisActor.countCustomLights(n) > 0)
        {
            sw_label.insertLast("Light " + (n + 1)); sw_on.insertLast(thisActor.getCustomLightVisible(n)); sw_kind.insertLast(2); sw_n.insertLast(n);
        }
    }
    for (uint i = 0; i < sw_label.length(); i++)
    {
        const float bx = x_att + 8.f + (i % 2) * 68.f;
        const float by = 114.f + (i / 2) * 22.f;
        if (Button(bx, by, 64.f, 18.f, sw_label[i], sw_on[i], ""))
        {
            if (sw_kind[i] == 0)
            {
                thisActor.setHeadlightsVisible(!thisActor.getHeadlightsVisible());
                thisActor.setSideLightsVisible(thisActor.getHeadlightsVisible());
            }
            else if (sw_kind[i] == 1)
            {
                thisActor.beaconsToggle();
            }
            else
            {
                thisActor.setCustomLightVisible(sw_n[i], !thisActor.getCustomLightVisible(sw_n[i]));
            }
        }
    }
}

void main()
{
    LoadSettings();
}
