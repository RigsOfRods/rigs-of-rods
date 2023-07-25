// TUTORIAL SCRIPT -findResourceFileInfo()
// ===================================================

const string RG_NAME = "Scripts";
const string RG_PATTERN = "demo*.*";

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    array<dictionary> @fileinfo = game.findResourceFileInfo(RG_NAME, RG_PATTERN);
    if (@fileinfo != null)
    {
        ImGui::Text(RG_NAME+" ("+fileinfo.length()+" results for pattern '"+RG_PATTERN+"')");
        for (uint i=0; i<fileinfo.length(); i++)
        {
            string filename = string(fileinfo[i]['filename']);  // get `string` from dict
            uint size = uint(fileinfo[i]['compressedSize']); // get `uint` from dict
            ImGui::Bullet(); 
            ImGui::SameLine(); ImGui::Text(filename);
            ImGui::SameLine(); ImGui::TextDisabled("("+float(size)/1000.f+" KB)");
        }
    }
    else
    {
        ImGui::Text("Null result :(");
    }
}
