// TEST SCRIPT 
// - new `const int thisScript` global var
// - and new `game` funcs:
//     * getScriptDetails()
//     * getRunningScripts()
// ===================================


// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    array<int> nids = game.getRunningScripts();
    ImGui::TextDisabled("Running scripts ("+nids.length()+', our NID:'+thisScript+'):');
   for (uint i=0; i<nids.length(); i++)
   {
       ImGui::Bullet(); ImGui::SameLine(); ImGui::TextDisabled('['+i+'] ');
       dictionary@ info = game.getScriptDetails(nids[i]);
       ImGui::SameLine();
       if (@info == null) { ImGui::Text('null'); }
       else {
           ImGui::Text(''+int( info['uniqueId'] ) +' >> '
                + string(info['scriptName']) + ' (' + ScriptCategory(info['scriptCategory']) + ')');
       }
       if (nids[i] == thisScript) { ImGui::SameLine(); ImGui::Text('<------- THIS SCRIPT!'); }
    }
}
