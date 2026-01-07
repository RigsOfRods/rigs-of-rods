
// \title Manual vehicle controls
// \brief Demo of functions to manually control vehicles in AngelScript
// ===================================================

#include "imgui_utils.as"

// Window [X] button handler
imgui_utils::CloseWindowPrompt closeBtnHandler;

void ShowActorControlTypeFlag(ActorControlTypeFlags&inout flags, string flagName, ActorControlTypeFlags value)
{
    bool enableFlag = (flags & value) > 0;
    if (ImGui::Checkbox(flagName, enableFlag))
    {
        if (enableFlag)
            flags = ActorControlTypeFlags(flags | value);
        else
            flags = ActorControlTypeFlags(flags & ~value);
    }
}

float engineThrottle = 0;
void ShowThrottleControl(BeamClass@ vehicle)
{
    int truckType = vehicle.getTruckType();
    if (truckType == TT_TRUCK)
    {
        EngineClass@ engine = vehicle.getEngine();
        if (@engine != null)
        {
            if (ImGui::SliderFloat("Throttle", engineThrottle, 0, 100))
                engine.autoSetAcc(engineThrottle / 100);
        }
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

void ShowClutchControl(BeamClass@ vehicle)
{
    EngineClass@ engine = vehicle.getEngine();
    if (@engine != null)
    {
        float clutch = engine.getClutch() * 100;
        if (ImGui::SliderFloat("Clutch", clutch, 0, 100))
            engine.setClutch(clutch / 100);
    }
}

void ShowBrakeControl(BeamClass@ vehicle)
{
    float brake = vehicle.getBrakingLevel() * 100;
    if (ImGui::SliderFloat("Brake", brake, 0, 100))
        vehicle.setBrakingLevel(brake / 100);
}

void ShowSteeringControl(BeamClass@ vehicle)
{
    float steering = vehicle.getSteeringAngle();
    if (ImGui::SliderFloat("Steering", steering, -1, 1))
        vehicle.setSteeringAngle(steering);
}

void ShowRudderControl(BeamClass@ vehicle)
{
    int truckType = vehicle.getTruckType();
    if (truckType == TT_AIRPLANE)
    {
        float rudder = vehicle.getAircraftRudder();
        if (ImGui::SliderFloat("Rudder", rudder, -1, 1))
            vehicle.setAircraftRudder(rudder);
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
    float aileron = vehicle.getAircraftAileron();
    if (ImGui::SliderFloat("Aileron", aileron, -1, 1))
        vehicle.setAircraftAileron(aileron);

    float elevator = vehicle.getAircraftElevator();
    if (ImGui::SliderFloat("Elevator", elevator, -1, 1))
        vehicle.setAircraftElevator(elevator);

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
            ImGui::Text("Vehicle: " + vehicle.getTruckName());

            ImGui::Separator();

            ActorControlTypeFlags linkedControls = vehicle.getControlsLinkedToExternalInput();
            ImGui::Text("Controls linked to external inputs (i.e. keyboard, controller, autopilot, etc.)");
            ShowActorControlTypeFlag(linkedControls, "ACT_THROTTLE", ACT_THROTTLE);
            ShowActorControlTypeFlag(linkedControls, "ACT_CLUTCH", ACT_CLUTCH);
            ShowActorControlTypeFlag(linkedControls, "ACT_BRAKE", ACT_BRAKE);
            ShowActorControlTypeFlag(linkedControls, "ACT_STEERING", ACT_STEERING);
            ShowActorControlTypeFlag(linkedControls, "ACT_AILERON", ACT_AILERON);
            ShowActorControlTypeFlag(linkedControls, "ACT_ELEVATOR", ACT_ELEVATOR);
            ShowActorControlTypeFlag(linkedControls, "ACT_RUDDER", ACT_RUDDER);
            ShowActorControlTypeFlag(linkedControls, "ACT_ALL_CONTROLS", ACT_ALL_CONTROLS);
            vehicle.setControlsLinkedToExternalInput(linkedControls);

            ImGui::Text("Controls");

            ShowThrottleControl(vehicle);
            ShowClutchControl(vehicle);
            ShowBrakeControl(vehicle);
            ShowSteeringControl(vehicle);

            ImGui::Separator();

            ShowRudderControl(vehicle);
            ShowAircraftControls(vehicle);
            ShowAircraftEngineControls(vehicle);

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