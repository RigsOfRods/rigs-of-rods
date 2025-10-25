
// \title Screwprops
// \brief Demo of ScrewpropClass binding
// ===================================================

#include "imgui_utils.as"

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

string YesOrNo(bool b)
{
    return b ? "Yes" : "No";
}

string RudderDeflection(float rudderDefl)
{
    string direction;
    if (rudderDefl == 0)
        direction = "centre";
    else
        direction = rudderDefl > 0 ? "left" : "right";
    return formatFloat(rudderDefl * 100, '', 0, 1) + " %% (" + direction + ")";
}

void frameStep(float dt)
{
    ImGui::SetNextWindowSize(vector2(440, 520));
    bool windowDummy = true;
    if (ImGui::Begin("Screwprops", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        BeamClass@ truck = game.getCurrentTruck();
        if (@truck != null)
        {
            ImGui::Text("Vehicle: " + truck.getTruckName());
            int screwpropCount = truck.getScrewpropCount();
            ImGui::Text("Available screwprops: " + formatInt(screwpropCount));
            
            if (screwpropCount > 0)
            {
                for (int i = 0; i < screwpropCount; i++)
                {
                    ImGui::Separator();
                    ScrewpropClass@ screwprop = truck.getScrewprop(i);

                    ImGui::Text("Screwprop #" + formatInt(i + 1) + ":");
                    ImGui::BulletText("Throttle: " + formatFloat(screwprop.getThrottle() * 100, '', 0, 1) + " %%");
                    ImGui::BulletText("Rudder deflection: " + RudderDeflection(screwprop.getRudder()));
                    ImGui::BulletText("Max force: " + formatFloat(screwprop.getMaxPower() / 1000, '', 0, 1) + " kN");
                    ImGui::BulletText("Reverser: " + YesOrNo(screwprop.getReverse()));
                    ImGui::BulletText("Reference node: " + formatInt(screwprop.getRefNode()));
                    ImGui::BulletText("Back node: " + formatInt(screwprop.getBackNode()));
                    ImGui::BulletText("Up node: " + formatInt(screwprop.getUpNode()));

                    float throttle = screwprop.getThrottle() * 100;
                    float rudder = screwprop.getRudder() * 100;
                    bool reverse = screwprop.getReverse();

                    ImGui::PushID("THR" + formatInt(i));
                    if (ImGui::SliderFloat("Set throttle", throttle, -100, 100))
                        screwprop.setThrottle(throttle / 100);
                    ImGui::PopID();

                    ImGui::PushID("RUDD" + formatInt(i));
                    if (ImGui::SliderFloat("Rudder deflection", rudder, -100, 100))
                        screwprop.setRudder(rudder / 100);
                    ImGui::PopID();

                    ImGui::PushID("REVB" + formatInt(i));
                    if (ImGui::Button("Toggle reverser"))
                        screwprop.toggleReverse();
                    ImGui::PopID();

                    ImGui::PushID("CENTRERUD" + formatInt(i));
                    if (ImGui::Button("Centre rudder"))
                        screwprop.setRudder(0);
                    ImGui::PopID();
                }
            }
        }
        else
        {
            ImGui::Text("You are swimming.");
        }

        ImGui::End();
    }
}