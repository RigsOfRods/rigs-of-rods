// \title Autopilot
// \brief Demo of AutopilotClass binding
// ===================================================

#include "imgui_utils.as"

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

void frameStep(float dt)
{
    ImGui::SetNextWindowSize(vector2(600, 500));
    if (ImGui::Begin("Autopilot", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        BeamClass@ truck = game.getCurrentTruck();
        if (@truck != null)
        {
            int nodeCount = truck.getNodeCount();
            ImGui::Text("Vehicle: " + truck.getTruckName());

            ImGui::Separator();
            
            AutopilotClass@ autopilot = truck.getAutopilot();
            if (@autopilot != null)
            {
                // Heading modes
                bool headingFixed = autopilot.getHeadingMode() == HEADING_FIXED;
                if (ImGui::Checkbox("HDG", headingFixed))
                    autopilot.toggleHeading(headingFixed ? HEADING_FIXED : HEADING_NONE);

                int heading = autopilot.getHeadingValue();
                if (ImGui::InputInt("Heading", heading))
                    autopilot.adjustHeading(heading - autopilot.getHeadingValue());

                bool headingNAV = autopilot.getHeadingMode() == HEADING_NAV;
                if (ImGui::Checkbox("NAV", headingNAV))
                    autopilot.toggleHeading(headingNAV ? HEADING_NAV : HEADING_NONE);

                bool headingWingLeveler = autopilot.getHeadingMode() == HEADING_WLV;
                if (ImGui::Checkbox("WLV (wing leveler)", headingWingLeveler))
                    autopilot.toggleHeading(headingWingLeveler ? HEADING_WLV : HEADING_NONE);

                // Altitude modes
                bool altFixed = autopilot.getAltitudeMode() == ALT_FIXED;
                if (ImGui::Checkbox("HOLD", altFixed))
                    autopilot.toggleAltitude(altFixed ? ALT_FIXED : ALT_NONE);

                int altitude = autopilot.getAltitudeValue();
                if (ImGui::InputInt("Altitude", altitude))
                    autopilot.adjustAltitude(altitude - autopilot.getAltitudeValue());

                bool altVS = autopilot.getAltitudeMode() == ALT_VS;
                if (ImGui::Checkbox("V/S", altVS))
                    autopilot.toggleAltitude(altVS ? ALT_VS : ALT_NONE);
                    
                int verticalSpeed = autopilot.getVerticalSpeedValue();
                if (ImGui::InputInt("Vertical speed", verticalSpeed))
                    autopilot.adjustVerticalSpeed(verticalSpeed - autopilot.getVerticalSpeedValue());
                
                string autoThrLabel = autopilot.getIASMode() ? "Disable auto-throttle" : "Enable auto-throttle";
                if (ImGui::Button(autoThrLabel))
                    autopilot.toggleIAS();

                int iasForATHR = autopilot.getIASValue();
                if (ImGui::InputInt("Airspeed reference for auto-throttle", iasForATHR))
                    autopilot.adjustIAS(iasForATHR - autopilot.getIASValue());
                
                string gpwsLabel = autopilot.getGPWSMode() ? "Disable GPWS" : "Enable GPWS";
                if (ImGui::Button(gpwsLabel))
                    autopilot.toggleGPWS();

                if (autopilot.isILSAvailable())
                {
                    ImGui::Text("ILS:");
                    ImGui::BulletText("Localizer deviation: " + formatFloat(autopilot.getHorizontalApproachDeviation(), '', 0, 1) + "°");
                    ImGui::BulletText("Glideslope deviation: " + formatFloat(autopilot.getVerticalApproachDeviation(), '', 0, 1) + "°");
                }
                else
                {
                    ImGui::Text("ILS is not available");
                }

                bool canOperateControls = autopilot.getOperateControls();
                if (ImGui::Checkbox("Operate controls", canOperateControls))
                    autopilot.setOperateControls(canOperateControls);

                if (ImGui::Button("Disconnect A/P"))
                    autopilot.disconnect();
            }
            else
            {
                ImGui::Text("This vehicle doesn't have an autopilot.");
            }
        }
        else
        {
            ImGui::Text("You are on foot.");
        }

        ImGui::End();
    }
}