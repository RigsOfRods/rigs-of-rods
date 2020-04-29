Ogre::TexturePtr G_icon_turbo;
Ogre::TexturePtr G_icon_tacho_4000;
Ogre::TexturePtr G_icon_tacho_4000_mph;
Ogre::TexturePtr G_icon_tacho_8000;
Ogre::TexturePtr G_icon_tacho_8000_mph;
Ogre::TexturePtr G_icon_needle_part1;
Ogre::TexturePtr G_icon_needle_part2;
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

    G_icon_turbo             = Ogre::TextureManager::getSingleton().load("turbo_v2.png",                 rg);
    G_icon_tacho_4000        = Ogre::TextureManager::getSingleton().load("tacho4000_digital_v2.png",     rg);
    G_icon_tacho_4000_mph    = Ogre::TextureManager::getSingleton().load("tacho4000_digital_v2_mph.png", rg);
    G_icon_tacho_8000        = Ogre::TextureManager::getSingleton().load("tacho8000_digital_v2.png",     rg);
    G_icon_tacho_8000_mph    = Ogre::TextureManager::getSingleton().load("tacho8000_digital_v2_mph.png", rg);
    G_icon_needle_part1      = Ogre::TextureManager::getSingleton().load("redneedle_v2.png",             rg);
    G_icon_needle_part2      = Ogre::TextureManager::getSingleton().load("needle_prt2.png",              rg);
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

int loop(GfxActor@ actor)
{
    ActorSimBuffer@ data = actor.GetSimDataBuffer();

    ImGui::SetNextWindowSize(vector2(640, 545));
    ImGui::Begin("Default HUD", true);


    ImGui::SetCursorPos(vector2(232,38));
    ImGui::Image(G_icon_turbo, vector2(112, 112));

    ImGui::SetCursorPos(vector2(238,42));
    ImGui::Image(G_icon_needle_part1, vector2(99, 99));

    ImGui::SetCursorPos(vector2(280,88));
    ImGui::Image(G_icon_needle_part2, vector2(15, 15));

    ImGui::SetCursorPosY(138);
    ImGui::Image(G_icon_tacho_4000, vector2(333, 250));

    ImGui::SetCursorPos(vector2(162,208));
    ImGui::Image(data.turn_signal_left ? G_icon_signal_left_on : G_icon_signal_left_off, vector2(31, 40));

    ImGui::SetCursorPos(data.turn_signal_right ? vector2(242,206) : vector2(244,207));
    ImGui::Image(data.turn_signal_right ? G_icon_signal_right_on : G_icon_signal_right_off, vector2(31, 40));

    ImGui::SetCursorPos(vector2(190,200));
    ImGui::Image(G_icon_brake_off, vector2(55, 55));

    ImGui::SetCursorPos(vector2(94,140));
    ImGui::Image(G_icon_needle_part1, vector2(248, 248));

    ImGui::SetCursorPos(vector2(202,246));
    ImGui::Image(G_icon_needle_part2, vector2(32, 32));

    ImGui::SetCursorPos(vector2(58,270));
    ImGui::Image(G_icon_lights_off, vector2(35, 35));

    ImGui::SetCursorPos(vector2(38,316));
    ImGui::Image(G_icon_secure_off, vector2(25, 25));

    ImGui::SetCursorPos(vector2(70,312));
    ImGui::Image(G_icon_locked0, vector2(30, 30));

    ImGui::SetCursorPos(vector2(22,354));
    ImGui::Image(G_icon_clutch_off, vector2(25, 25));

    ImGui::SetCursorPos(vector2(48,354));
    ImGui::Image(G_icon_lopress_off, vector2(25, 25));

    ImGui::SetCursorPos(vector2(76,348));
    ImGui::Image(G_icon_battery_off, vector2(35, 35));

    ImGui::SetCursorPos(vector2(112,348));
    ImGui::Image(G_icon_ign_off, vector2(35, 35));

    ImGui::SetCursorPos(vector2(192,300));
    ImGui::Image(G_icon_tractioncontrol1, vector2(25, 25));

    ImGui::SetCursorPos(vector2(222,300));
    ImGui::Image(G_icon_abs1, vector2(25, 25));

    ImGui::End();
    
    return 0;
}
