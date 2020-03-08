
CVar@ sim_gearbox_mode = null;
CVar@ sim_terrain_name = null;
CVar@ gfx_enable_videocams = null;

int setup(string arg)
{
    @sim_gearbox_mode     = RoR::GetConsole().CVarFind("sim_gearbox_mode");
    @sim_terrain_name     = RoR::GetConsole().CVarFind("sim_terrain_name");
    @gfx_enable_videocams = RoR::GetConsole().CVarFind("gfx_enable_videocams");
    return 0;
}

int loop(GfxActor@ actor)
{
    string str;
    ActorSimBuffer@ data = actor.GetSimDataBuffer();

    ImGui::Begin("Scripting test", true);

    ImGui::Separator();
    ImGui::Text("SimBuffer:");
    
        str = "> Pos: X" + data.pos.x + ", Y" + data.pos.y + ", Z" + data.pos.z;
        ImGui::Text(str);
    
    
    ImGui::Separator();
    ImGui::Text("CVars:");
    
        str = "> gearbox mode:" + sim_gearbox_mode.GetActiveInt();
        ImGui::Text(str);
        
        str = "> terrain name:" + sim_terrain_name.GetActiveStr();
        ImGui::Text(str);
        
        str = "> videocams on:" + gfx_enable_videocams.GetActiveBool();
        ImGui::Text(str);

    ImGui::End();
    return 0;
}
