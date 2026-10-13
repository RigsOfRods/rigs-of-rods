// \title Manual vehicle controls
// \brief Demo of functions to manually control vehicles in AngelScript
// ===================================================

#include "imgui_utils.as"

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

void ShowGenericControl(BeamClass@ vehicle, inputEvents input, string sliderCaption)
{
    float val = vehicle.getEventSimulatedValue(input) * 100;
    if (ImGui::SliderFloat(sliderCaption, val, 0, 100))
    {
        if (val > 0)
            vehicle.setEventSimulatedValue(input, val / 100);
        else
            vehicle.resetEventSimulatedValue(input);
    }
}

void ShowLeftRightControl(BeamClass@ vehicle, inputEvents left, inputEvents right, string sliderCaption)
{
    float sum =
        (vehicle.getEventSimulatedValue(right) - vehicle.getEventSimulatedValue(left)) * 100;
    if (ImGui::SliderFloat(sliderCaption, sum, -100, 100))
    {
        if (sum > 0)
        {
            vehicle.setEventSimulatedValue(right, sum / 100);
        }
        else if (sum < 0)
        {
            vehicle.setEventSimulatedValue(left, -sum / 100);
        }
        else
        {
            vehicle.resetEventSimulatedValue(right);
            vehicle.resetEventSimulatedValue(left);
        }
    }

    if (ImGui::Button("Reset " + sliderCaption))
    {
        vehicle.resetEventSimulatedValue(right);
        vehicle.resetEventSimulatedValue(left);
    }
}

float engineThrottle = 0;
void ShowThrottleControl(BeamClass@ vehicle)
{
    int truckType = vehicle.getTruckType();
    if (truckType == TT_TRUCK)
    {
        ShowGenericControl(vehicle, EV_TRUCK_ACCELERATE, "Throttle");
    }
    else if (truckType == TT_AIRPLANE)
    {
        // Calculate average throttle value
        float throttle = 0;
        for (int i = 0; i < vehicle.getAircraftEngineCount(); i++)
        {
            AircraftEngineClass@ aircraftEng = vehicle.getAircraftEngine(i);
            throttle += aircraftEng.getThrottle();
        }
        throttle /= vehicle.getAircraftEngineCount();

        throttle *= 100;
        if (ImGui::SliderFloat("Throttle", throttle, 0, 100))
        {
            for (int i = 0; i < vehicle.getAircraftEngineCount(); i++)
            {
                AircraftEngineClass@ aircraftEng = vehicle.getAircraftEngine(i);
                aircraftEng.setThrottle(throttle / 100);
            }
        }
    }
    else if (truckType == TT_BOAT)
    {
        // Calculate average throttle value
        float throttle = 0;
        for (int i = 0; i < vehicle.getScrewpropCount(); i++)
        {
            ScrewpropClass@ screwprop = vehicle.getScrewprop(i);
            throttle += screwprop.getThrottle();
        }
        throttle /= vehicle.getScrewpropCount();

        throttle *= 100;
        if (ImGui::SliderFloat("Throttle", throttle, -100, 100))
        {
            for (int i = 0; i < vehicle.getScrewpropCount(); i++)
            {
                ScrewpropClass@ screwprop = vehicle.getScrewprop(i);
                screwprop.setThrottle(throttle / 100);
            }
        }
    }
}

void ShowRudderControl(BeamClass@ vehicle)
{
    int truckType = vehicle.getTruckType();
    if (truckType == TT_AIRPLANE)
    {
        ShowLeftRightControl(vehicle, EV_AIRPLANE_RUDDER_LEFT, EV_AIRPLANE_RUDDER_RIGHT, "Rudder");
    }
    else if (truckType == TT_BOAT)
    {
        // Calculate average rudder value
        float rudder = 0;
        for (int i = 0; i < vehicle.getScrewpropCount(); i++)
        {
            ScrewpropClass@ screwprop = vehicle.getScrewprop(i);
            rudder += screwprop.getRudder();
        }
        rudder /= vehicle.getScrewpropCount();

        if (ImGui::SliderFloat("Rudder", rudder, -1, 1))
        {
            for (int i = 0; i < vehicle.getScrewpropCount(); i++)
            {
                ScrewpropClass@ screwprop = vehicle.getScrewprop(i);
                screwprop.setRudder(rudder);
            }
        }
    }
}

void ShowAircraftControls(BeamClass@ vehicle)
{
    ShowLeftRightControl(vehicle, EV_AIRPLANE_STEER_LEFT, EV_AIRPLANE_STEER_RIGHT, "Aileron");
    ShowLeftRightControl(vehicle, EV_AIRPLANE_ELEVATOR_DOWN, EV_AIRPLANE_ELEVATOR_UP, "Elevator");

    ImGui::Text("Flaps: " + formatInt(vehicle.getAircraftFlaps()));
    if (ImGui::Button("More flaps"))
        vehicle.setAircraftFlaps(vehicle.getAircraftFlaps() + 1);
    if (ImGui::Button("Less flaps"))
        vehicle.setAircraftFlaps(vehicle.getAircraftFlaps() - 1);

    ImGui::Text("Air brakes: " + formatInt(int(vehicle.getAirbrakeIntensity())));
    if (ImGui::Button("More air braking"))
        vehicle.setAirbrakeIntensity(vehicle.getAirbrakeIntensity() + 1.0);
    if (ImGui::Button("Less air braking"))
        vehicle.setAirbrakeIntensity(vehicle.getAirbrakeIntensity() - 1.0);
}

void ShowAircraftEngineControls(BeamClass@ vehicle)
{
    int engCount = vehicle.getAircraftEngineCount();
    if (engCount > 0)
    {
        for (int i = 0; i < engCount; i++)
        {
            AircraftEngineClass@ engine = vehicle.getAircraftEngine(i);

            string engineNumStr = formatInt(i + 1);
            ImGui::Text("Aircraft engine #" + engineNumStr);
            
            string engineIgnitionLabel = engine.getIgnition() ? "Stop" : "Start";
            ImGui::PushID("ENGSTART" + engineNumStr);
            if (ImGui::Button(engineIgnitionLabel))
                engine.flipStart();
            
            string engineReverserLabel = (engine.getReverse() ? "Disengage" : "Engage") + " thrust reverser";
            ImGui::PushID("ENGREV" + engineNumStr);
            if (ImGui::Button(engineReverserLabel))
                engine.toggleReverse();
        }
    }
}

void ShowParkingBrakeControl(BeamClass@ vehicle)
{
    string btnLabel = vehicle.getParkingBrake() ? "Disengage parking brake" : "Engage parking brake";
    if (ImGui::Button(btnLabel))
        vehicle.parkingbrakeToggle();
}

void ShowIgnitionControl(BeamClass@ vehicle)
{
    EngineClass@ engine = vehicle.getEngine();
    if (@engine != null)
    {
        string btnLabel = engine.hasContact() ? "Disable ignition" : "Enable ignition";
        if (ImGui::Button(btnLabel))
            engine.toggleContact();
    }
}

void ShowStarterControl(BeamClass@ vehicle)
{
    EngineClass@ engine = vehicle.getEngine();
    if (@engine != null)
    {
        if (ImGui::Button("Start engine"))
            engine.startEngine();
    }
}

void ShowShiftingControls(BeamClass@ vehicle)
{
    EngineClass@ engine = vehicle.getEngine();
    if (@engine != null)
    {
        ImGui::Separator();

        ImGui::Text("Gearbox mode");
        SimGearboxMode mode = engine.getAutoMode();
        bool isAuto = mode == SimGearboxMode::AUTO;
        if (ImGui::RadioButton("Automatic", isAuto))
            mode = SimGearboxMode::AUTO;

        bool isSemiAuto = mode == SimGearboxMode::SEMI_AUTO;
        if (ImGui::RadioButton("Semi-automatic (manual shift with auto clutch)", isSemiAuto))
            mode = SimGearboxMode::SEMI_AUTO;

        bool isManual = mode == SimGearboxMode::MANUAL;
        if (ImGui::RadioButton("Manual", isManual))
            mode = SimGearboxMode::MANUAL;

        bool isManualStick = mode == SimGearboxMode::MANUAL_STICK;
        if (ImGui::RadioButton("Manual (stick shift)", isManualStick))
            mode = SimGearboxMode::MANUAL_STICK;

        bool isManualRanges = mode == SimGearboxMode::MANUAL_RANGES;
        if (ImGui::RadioButton("Manual, with ranges", isManualRanges))
            mode = SimGearboxMode::MANUAL_RANGES;

        engine.setAutoMode(mode);

        if (ImGui::Button("Shift up"))
            engine.shift(1);
        if (ImGui::Button("Shift down"))
            engine.shift(-1);

        ImGui::Separator();
    }
}

void ShowABSControl(BeamClass@ vehicle)
{
    string btnLabel = vehicle.getAntiLockBrake() ? "Disable ABS" : "Enable ABS";
    if (ImGui::Button(btnLabel))
        vehicle.antilockbrakeToggle();
}

void ShowTractionControl(BeamClass@ vehicle)
{
    string btnLabel = vehicle.getTractionControl() ? "Disable traction control" : "Enable traction control";
    if (ImGui::Button(btnLabel))
        vehicle.tractioncontrolToggle();
}

void ShowCruiseControl(BeamClass@ vehicle)
{
    string btnLabel = vehicle.getCruiseControl() ? "Disable cruise control" : "Enable cruise control";
    if (ImGui::Button(btnLabel))
        vehicle.cruisecontrolToggle();
}

void frameStep(float dt)
{
    ImGui::SetNextWindowSize(vector2(480, 640));
    if (ImGui::Begin("Manual Vehicle Controls", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        BeamClass@ vehicle = game.getCurrentTruck();
        if (@vehicle != null)
        {
            int truckType = vehicle.getTruckType();
            ImGui::Text("Vehicle: " + vehicle.getTruckName());
            ImGui::Text("Controls");

            ShowThrottleControl(vehicle);
            ShowGenericControl(vehicle, EV_TRUCK_MANUAL_CLUTCH, "Clutch");
            ShowGenericControl(vehicle, EV_TRUCK_BRAKE, "Brake");
            if (truckType == TT_TRUCK)
                ShowLeftRightControl(vehicle, EV_TRUCK_STEER_LEFT, EV_TRUCK_STEER_RIGHT, "Steering");

            ImGui::Separator();

            ShowRudderControl(vehicle);
            if (truckType == TT_AIRPLANE)
            {
                ShowAircraftControls(vehicle);
                ShowAircraftEngineControls(vehicle);
            }

            ImGui::Separator();

            ShowParkingBrakeControl(vehicle);
            ShowIgnitionControl(vehicle);
            ShowStarterControl(vehicle);
            ShowShiftingControls(vehicle);
            ShowABSControl(vehicle);
            ShowTractionControl(vehicle);
            ShowCruiseControl(vehicle);
        }
        else
        {
            engineThrottle = 0;
            ImGui::Text("You are on foot.");
        }

        ImGui::End();
    }
}