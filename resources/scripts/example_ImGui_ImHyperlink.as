// EXAMPLE SCRIPT - how to display hyperlink.
// ===================================================

#include "imgui_utils.as"  // ImHyperlink()

void frameStep(float dt)
{
    ImHyperlink("http://developer.rigsofrods.org", "Dev portal");
}
