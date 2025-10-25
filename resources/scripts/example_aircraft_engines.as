// \title Aircraft engines
// \brief Demo of new bindings for aircraft engines (AircraftEngineClass, TurbojetClass, TurbopropClass)
// ===================================================

#include "imgui_utils.as"

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

string YesOrNo(bool b)
{
    return b ? "Yes" : "No";
}

void ShowEngineType(AircraftEngineClass@ engine)
{
    string engType = "";
    switch (engine.getType())
    {
        case AE_TURBOJET:
            engType = "Turbojet";
            break;
        case AE_PROPELLER:
            engType = "Propeller";
            break;
        default:
            engType = "Unknown";
            break;
    }

    ImGui::BulletText("Type: " + engType);
}

void frameStep(float dt)
{
    ImGui::SetNextWindowSize(vector2(440, 520));
    bool windowDummy = true;
    if (ImGui::Begin("Aircraft engines", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        BeamClass@ truck = game.getCurrentTruck();
        if (@truck != null)
        {
            ImGui::Text("Vehicle: " + truck.getTruckName());
            int engCount = truck.getAircraftEngineCount();
            ImGui::Text("Available aircraft engines: " + formatInt(engCount));
            
            if (engCount > 0)
            {
                for (int i = 0; i < engCount; i++)
                {
                    ImGui::Separator();
                    AircraftEngineClass@ engine = truck.getAircraftEngine(i);

                    ImGui::Text("Engine #" + formatInt(i + 1) + ":");
                    ShowEngineType(engine);
                    ImGui::BulletText("Throttle: " + formatFloat(engine.getThrottle() * 100, '', 0, 1) + " %%");
                    ImGui::BulletText("RPM %%: " + formatFloat(engine.getRPMPercent(), '', 0, 1) + " %%");
                    ImGui::BulletText("Failed: " + YesOrNo(engine.isFailed()));
                    ImGui::BulletText("Ignition: " + YesOrNo(engine.getIgnition()));
                    ImGui::BulletText("Warmup: " + YesOrNo(engine.getWarmup()));
                    ImGui::BulletText("Reverse thrust: " + YesOrNo(engine.getReverse()));
                    ImGui::BulletText("Front node: " + formatInt(engine.getFrontNode()));
                    ImGui::BulletText("Back node: " + formatInt(engine.getBackNode()));

                    float throttle = engine.getThrottle() * 100;
                    bool reverse = engine.getReverse();

                    ImGui::PushID("THR" + formatInt(i));
                    if (ImGui::SliderFloat("Set throttle", throttle, 0, 100))
                        engine.setThrottle(throttle / 100);
                    ImGui::PopID();
                        
                    ImGui::PushID("REV" + formatInt(i));
                    if (ImGui::Checkbox("Set reverse thrust", reverse))
                        engine.setReverse(reverse);
                    ImGui::PopID();

                    ImGui::PushID("REVB" + formatInt(i));
                    if (ImGui::Button("Toggle reverse"))
                        engine.toggleReverse();
                    ImGui::PopID();

                    ImGui::PushID("START" + formatInt(i));
                    string startBtnLabel = engine.getIgnition() ? "Stop" : "Start";
                    if (ImGui::Button(startBtnLabel))
                        engine.flipStart();
                    ImGui::PopID();

                    if (engine.getType() == AE_TURBOJET)
                    {
                        TurbojetClass@ turbojet = truck.getTurbojet(i);
                        ImGui::Text("Turbojet info:");
                        ImGui::BulletText("Dry thrust: " + formatFloat(turbojet.getMaxDryThrust(), '', 0, 1) + " kN");
                        ImGui::BulletText("Afterburner is active: " + YesOrNo(turbojet.getAfterburner()));
                        if (turbojet.getAfterburner())
                            ImGui::BulletText("Afterburner thrust: " + formatFloat(turbojet.getAfterburnerThrust(), '', 0, 1) + " kN");
                        ImGui::BulletText("Exhaust velocity: " + formatFloat(turbojet.getExhaustVelocity(), '', 0, 1) + " m/s");
                    }
                    else if (engine.getType() == AE_PROPELLER)
                    {
                        TurbopropClass@ propeller = truck.getTurboprop(i);
                        ImGui::Text("Propeller info:");
                        ImGui::BulletText("Pitch: " + formatFloat(propeller.getPropellerPitch(), '', 0, 1));
                        ImGui::BulletText("Torque: " + formatFloat(propeller.getPropellerIndicatedTorque(), '', 0, 1) + " N m");
                        ImGui::BulletText("Max torque: " + formatFloat(propeller.getPropellerMaxTorque(), '', 0, 1) + " N m");
                        ImGui::BulletText("Max power: " + formatFloat(propeller.getPropellerMaxPower(), '', 0, 1) + " kW");
                        ImGui::BulletText("Is piston prop: " + YesOrNo(propeller.isPistonProp()));
                    }
                }
            }
        }
        else
        {
            ImGui::Text("You are on foot.");
        }

        ImGui::End();
    }
}