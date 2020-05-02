Ogre::TexturePtr G_turbo_gauge;
Ogre::TexturePtr G_icon_tacho_4000;
Ogre::TexturePtr G_icon_tacho_4000_mph;
Ogre::TexturePtr G_icon_tacho_8000;
Ogre::TexturePtr G_icon_tacho_8000_mph;
Ogre::TexturePtr G_pointer_needle;
Ogre::TexturePtr G_pointer_center;
Ogre::TexturePtr G_icon_brake_off;
Ogre::TexturePtr G_icon_brake_on;
Ogre::TexturePtr G_icon_signal_left_off;
Ogre::TexturePtr G_icon_signal_left_on;
Ogre::TexturePtr G_icon_signal_right_off;
Ogre::TexturePtr G_icon_signal_right_on;
Ogre::TexturePtr G_icon_lights_off;
Ogre::TexturePtr G_icon_lights_on;
Ogre::TexturePtr G_icon_secure_off;
Ogre::TexturePtr G_icon_secure_on;
Ogre::TexturePtr G_icon_clutch_off;
Ogre::TexturePtr G_icon_clutch_on;
Ogre::TexturePtr G_icon_lopress_off;
Ogre::TexturePtr G_icon_lopress_on;
Ogre::TexturePtr G_icon_battery_off;
Ogre::TexturePtr G_icon_battery_on;
Ogre::TexturePtr G_icon_ign_off;
Ogre::TexturePtr G_icon_ign_on;
Ogre::TexturePtr G_icon_tractioncontrol1;
Ogre::TexturePtr G_icon_tractioncontrol2;
Ogre::TexturePtr G_icon_tractioncontrol3;
Ogre::TexturePtr G_icon_abs1;
Ogre::TexturePtr G_icon_abs2;
Ogre::TexturePtr G_icon_abs3;
Ogre::TexturePtr G_icon_locked0;
Ogre::TexturePtr G_icon_locked1;
Ogre::TexturePtr G_icon_locked2;

int setup(string arg)
{
    string rg = "DashboardsRG";

    G_turbo_gauge            = Ogre::TextureManager::getSingleton().load("turbo_v2.png",                 rg);
    G_icon_tacho_4000        = Ogre::TextureManager::getSingleton().load("tacho4000_digital_v2.png",     rg);
    G_icon_tacho_4000_mph    = Ogre::TextureManager::getSingleton().load("tacho4000_digital_v2_mph.png", rg);
    G_icon_tacho_8000        = Ogre::TextureManager::getSingleton().load("tacho8000_digital_v2.png",     rg);
    G_icon_tacho_8000_mph    = Ogre::TextureManager::getSingleton().load("tacho8000_digital_v2_mph.png", rg);
    G_pointer_needle         = Ogre::TextureManager::getSingleton().load("redneedle_v2.png",             rg);
    G_pointer_center         = Ogre::TextureManager::getSingleton().load("needle_prt2.png",              rg);
    G_icon_brake_off         = Ogre::TextureManager::getSingleton().load("pbrake_v2-off.png",            rg);
    G_icon_brake_on          = Ogre::TextureManager::getSingleton().load("pbrake_v2-on.png",             rg);
    G_icon_signal_left_off   = Ogre::TextureManager::getSingleton().load("turn_signal_left-off.png",     rg);
    G_icon_signal_left_on    = Ogre::TextureManager::getSingleton().load("turn_signal_left-on.png",      rg);
    G_icon_signal_right_off  = Ogre::TextureManager::getSingleton().load("turn_signal_right-off.png",    rg);
    G_icon_signal_right_on   = Ogre::TextureManager::getSingleton().load("turn_signal_right-on.png",     rg);
    G_icon_lights_off        = Ogre::TextureManager::getSingleton().load("lights_v2-off.png",            rg);
    G_icon_lights_on         = Ogre::TextureManager::getSingleton().load("lights_v2-on.png",             rg);
    G_icon_secure_off        = Ogre::TextureManager::getSingleton().load("secured_v2-off.png",           rg);
    G_icon_secure_on         = Ogre::TextureManager::getSingleton().load("secured_v2-on.png",            rg);
    G_icon_clutch_off        = Ogre::TextureManager::getSingleton().load("clutch_v2-off.png",            rg);
    G_icon_clutch_on         = Ogre::TextureManager::getSingleton().load("clutch_v2-on.png",             rg);
    G_icon_lopress_off       = Ogre::TextureManager::getSingleton().load("lopress_v2-off.png",           rg);
    G_icon_lopress_on        = Ogre::TextureManager::getSingleton().load("lopress_v2-on.png",            rg);
    G_icon_battery_off       = Ogre::TextureManager::getSingleton().load("batt_v2-off.png",              rg);
    G_icon_battery_on        = Ogre::TextureManager::getSingleton().load("batt_v2-on.png",               rg);
    G_icon_ign_off           = Ogre::TextureManager::getSingleton().load("ign_v2-off.png",               rg);
    G_icon_ign_on            = Ogre::TextureManager::getSingleton().load("ign_v2-on.png",                rg);
    G_icon_tractioncontrol1  = Ogre::TextureManager::getSingleton().load("tractioncontrol-1.png",        rg);
    G_icon_tractioncontrol2  = Ogre::TextureManager::getSingleton().load("tractioncontrol-2.png",        rg);
    G_icon_tractioncontrol3  = Ogre::TextureManager::getSingleton().load("tractioncontrol-3.png",        rg);
    G_icon_abs1              = Ogre::TextureManager::getSingleton().load("antilockbrake_v2-1.png",       rg);
    G_icon_abs2              = Ogre::TextureManager::getSingleton().load("antilockbrake_v2-2.png",       rg);
    G_icon_abs3              = Ogre::TextureManager::getSingleton().load("antilockbrake_v2-3.png",       rg);
    G_icon_locked0           = Ogre::TextureManager::getSingleton().load("locked_v2-0.png",              rg);
    G_icon_locked1           = Ogre::TextureManager::getSingleton().load("locked_v2-1.png",              rg);
    G_icon_locked2           = Ogre::TextureManager::getSingleton().load("locked_v2-2.png",              rg);

    return 0;
}

float PI = 3.141592;

// Calculate angle in radians for gauge needle display
// Params `gauge_min/gauge_max` are min/max output rotation in radians, relative to standby rotation.
// Param `gauge_bias` is a standby rotation in radians, useful when needle position in texture doesn't match idle gauge.
// Params `input_min/input_max` is a range of input values which maps to `gauge_min/gauge_max`
// Finally, param `input` is the value coming from simulation.
float CalcAngle(float gauge_min, float gauge_max, float gauge_bias, float input_min, float input_max, float input)
{
    return (input / ((input_max - input_min))) * (gauge_max - gauge_min) + gauge_min + gauge_bias;
}

int loop(GfxActor@ actor)
{
    ActorSimBuffer@ data = actor.GetSimDataBuffer();

    ImGui::SetNextWindowSize(vector2(640, 545));
    ImGui::Begin("Default HUD", true);

    /*  //DEBUG - TURBO (PSI)
    ImGui::SetCursorPos(vector2(232,26));
    ImGui::Text("PSI: " + data.engine_turbo_psi);
    */

    // Turbo gauge
    ImGui::SetCursorPos(vector2(232,38));
    ImGui::Image(G_turbo_gauge, vector2(112, 112));
    
    float turbo_angle = CalcAngle(0.27*PI, 1.7*PI, PI, 0, 25, data.engine_turbo_psi); // Needle texture points upwars, we need idle downwards -> bias = PI
    vector2 turbo_center = ImGui::GetCursorScreenPos() + vector2(278, -60);
    ImGuiEx::DrawListAddImageRotated(G_pointer_needle, turbo_center, vector2(99, 99), turbo_angle);    

    ImGui::SetCursorPos(vector2(280,88));
    ImGui::Image(G_pointer_center, vector2(15, 15));

    // Tacho gauge with lamps
    ImGui::SetCursorPosY(138);
    ImGui::Image(G_icon_tacho_4000, vector2(333, 250));
    
    /*    // DEBUG - RPM
    ImGui::SetCursorPos(vector2(100, 100));
    ImGui::Text("RPM: " + data.engine_rpm);    
    */

    ImGui::SetCursorPos(vector2(162,208));
    ImGui::Image(data.turn_signal_left ? G_icon_signal_left_on : G_icon_signal_left_off, vector2(31, 40));

    ImGui::SetCursorPos(data.turn_signal_right ? vector2(242,206) : vector2(244,207));
    ImGui::Image(data.turn_signal_right ? G_icon_signal_right_on : G_icon_signal_right_off, vector2(31, 40));

    ImGui::SetCursorPos(vector2(190,200));
    ImGui::Image(data.parking_brake ? G_icon_brake_on : G_icon_brake_off, vector2(55, 55));
    
    ImGui::SetCursorPos(vector2(123,167));
    float tacho_angle = CalcAngle(0, 1.3*PI, PI, 0, 4000, data.engine_rpm);
    vector2 tacho_center = ImGui::GetCursorScreenPos() + vector2(95, 95);
    ImGuiEx::DrawListAddImageRotated(G_pointer_needle, tacho_center, vector2(245, 245),  tacho_angle);    

    ImGui::SetCursorPos(vector2(202,246));
    ImGui::Image(G_pointer_center, vector2(32, 32));

    // Lamps panel
    ImGui::SetCursorPos(vector2(58,270));
    ImGui::Image(data.headlight_on ? G_icon_lights_on : G_icon_lights_off, vector2(35, 35));

    ImGui::SetCursorPos(vector2(38,316));
    ImGui::Image(data.hook_locked ? G_icon_secure_on : G_icon_secure_off, vector2(25, 25));

    ImGui::SetCursorPos(vector2(70,312));
    Ogre::TexturePtr secured_icon = (data.ties_secured_state == 0) ? G_icon_locked0 : 
                                    (data.ties_secured_state == 1) ? G_icon_locked1 : G_icon_locked2;
    ImGui::Image(secured_icon, vector2(30, 30));

    bool clutch_warning = abs(data.clutch_torque) >= data.clutch_force * 10.0f;
    ImGui::SetCursorPos(vector2(22,354));
    ImGui::Image(clutch_warning ? G_icon_clutch_on : G_icon_clutch_off, vector2(25, 25));

    ImGui::SetCursorPos(vector2(48,354));
    ImGui::Image(data.hydropump_ready ? G_icon_lopress_off : G_icon_lopress_on, vector2(25, 25));
    
    bool starter = data.engine_ignition && !data.engine_running;

    ImGui::SetCursorPos(vector2(76,348));
    ImGui::Image(starter ? G_icon_battery_on : G_icon_battery_off, vector2(35, 35));

    ImGui::SetCursorPos(vector2(112,348));
    ImGui::Image(starter ? G_icon_ign_on : G_icon_ign_off, vector2(35, 35));

    if (data.tc_dashboard_mode != 0)
    {
        ImGui::SetCursorPos(vector2(192,300));
        Ogre::TexturePtr tc_icon = (data.tc_dashboard_mode == 1) ? G_icon_tractioncontrol1 : 
                                   (data.tc_dashboard_mode == 2) ? G_icon_tractioncontrol2 : G_icon_tractioncontrol3;
        ImGui::Image(tc_icon, vector2(25, 25));
    }
    
    if (data.alb_dashboard_mode != 0)
    {
        ImGui::SetCursorPos(vector2(222,300));
        Ogre::TexturePtr alb_icon = (data.alb_dashboard_mode == 1) ? G_icon_abs1 :         
                                    (data.alb_dashboard_mode == 2) ? G_icon_abs2 : G_icon_abs3;
        ImGui::Image(alb_icon, vector2(25, 25));
    }

    ImGui::End();
    
    return 0;
}
