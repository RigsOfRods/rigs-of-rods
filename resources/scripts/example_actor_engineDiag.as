/// \title Engine + Clutch diag script
/// \brief Shows config params and state of engine simulation

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// #region FrameStep - clutch calc

vector2 cfgPlotSize(0.f, 45.f); // 0=autoresize
bool cfgUseGlobalGearForThresholdCalc = false; // by default 1st gear is used, which may be a bug.

// assume 60FPS = circa 3 sec
const int MAX_SAMPLES = 3*60;
array<float> clutchBuf(MAX_SAMPLES, 0.f);
array<float> rpmBuf(MAX_SAMPLES, 0.f);
array<float> clutchTorqueBuf(MAX_SAMPLES, 0.f);
array<float> calcGearboxSpinnerBuf(MAX_SAMPLES, 0.f);
array<float> calcClutchTorqueBuf(MAX_SAMPLES, 0.f);
array<float> calcForceThresholdBuf(MAX_SAMPLES, 0.f);
array<float> calcClutchTorqueClampedBuf(MAX_SAMPLES, 0.f);
array<float> calcClutchTorqueFinalBuf(MAX_SAMPLES, 0.f);

void updateFloatBuf(array<float>@ buf, float f)
{
    buf.removeAt(0);
    buf.insertLast(f); 
}

void updateEnginePlotBuffers(EngineClass@ engine)
{
    // Engine state values
    updateFloatBuf(clutchBuf, engine.getClutch());
    updateFloatBuf(rpmBuf, engine.getRPM());
    updateFloatBuf(clutchTorqueBuf, engine.getTorque());
    
    // Replicating the clutch torque calculation
    float gearboxspinner = engine.getRPM() / engine.getGearRatio(engine.getGear());
    updateFloatBuf(calcGearboxSpinnerBuf, gearboxspinner);
    
    float clutchTorque = (gearboxspinner - engine.getWheelSpin()) * engine.getClutch() * engine.getClutchForce();
    updateFloatBuf(calcClutchTorqueBuf, clutchTorque);
    
    float forceThreshold = 1.5f * fmax(engine.getTorque(), engine.getEnginePower()) * fabs(engine.getGearRatio(cfgUseGlobalGearForThresholdCalc ? 0 : 1)); // gear 1=BUG? should probably be 0=global
    updateFloatBuf(calcForceThresholdBuf, forceThreshold);
    
    float clutchTorqueClamped = fclamp(clutchTorque, -forceThreshold, forceThreshold);
    updateFloatBuf(calcClutchTorqueClampedBuf, clutchTorqueClamped);
    
    float clutchTorqueFinal = clutchTorqueClamped * (1.f - fexp(-fabs(gearboxspinner - engine.getWheelSpin())));
    updateFloatBuf(calcClutchTorqueFinalBuf, clutchTorqueFinal);
}

// #endregion

// #region frameStep
void frameStep(float dt)
{
    
    if (ImGui::Begin("Engine Tool", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        // force minimum width
        ImGui::Dummy(vector2(250, 1));
        
        BeamClass@ playerVehicle = game.getCurrentTruck();
        if (@playerVehicle == null)
        {
            ImGui::Text("You are on foot.");
        }
        else
        {
            EngineClass@ engine = playerVehicle.getEngine();
            if (@engine == null)
            {
                ImGui::Text("Your vehicle doesn't have an engine");
            }
            else
            {
                updateEnginePlotBuffers(engine);
                
                drawEngineDiagUI(playerVehicle, engine);
            }
        }
        
        ImGui::End();
    }
}
// #endregion

// #region UI drawing helpers
void drawTableRow(string key, float val)
{
    ImGui::TextDisabled(key); ImGui::NextColumn(); ImGui::Text(formatFloat(val, "", 0, 3)); ImGui::NextColumn();
}

void drawTableRow(string key, int val)
{
    ImGui::TextDisabled(key); ImGui::NextColumn(); ImGui::Text(''+val); ImGui::NextColumn();
}

void drawTableRow(string key, bool val)
{
    ImGui::TextDisabled(key); ImGui::NextColumn(); ImGui::Text(val ? 'true' : 'false'); ImGui::NextColumn();
}

void drawTableRowPlot(string key, array<float>@ buf, float rangeMin, float rangeMax)
{
    ImGui::TextDisabled(key);
    ImGui::NextColumn(); 
    plotFloatBuf(buf, rangeMin, rangeMax); 
    ImGui::NextColumn();  
}

void plotFloatBuf(array<float>@ buf, float rangeMin, float rangeMax)
{
    //DOC: "void PlotLines(const string&in label, array<float>&in values, int values_count, int values_offset = 0, const string&in overlay_text = string(), float scale_min = FLT_MAX, float scale_max = FLT_MAX, vector2 graph_size = vector2(0,0))",
    ImGui::PlotLines("", buf, MAX_SAMPLES, 0, "", rangeMin, rangeMax, cfgPlotSize);
    ImGui::SameLine();
    float val = buf[buf.length()-1];
    ImGui::Text(formatFloat(val, "", 0, 3));     
}

void drawAttrInputRow(BeamClass@ actor, ActorSimAttr attr, string label)
{
    ImGui::TextDisabled(label);
    ImGui::NextColumn();
    float val = actor.getSimAttribute(attr);
    if (ImGui::InputFloat("##"+label, val))
    {
        actor.setSimAttribute(attr, val);
    }
    ImGui::NextColumn();
}

void drawAttributesCommonHelp()
{
    if (ImGui::CollapsingHeader("How to read attributes:"))
    {
        ImGui::TextDisabled("Each value is displayed twice (for debugging) from different sources:");
        ImGui::TextDisabled("1. the 'get***' value is what EngineClass object reports via `get***()` functions.");
        ImGui::TextDisabled("2. the UPPERCASE value is what ActorClass object reports via `getSimAttribute()` function.");
        ImGui::TextDisabled("There are exceptions, a few values have multiple EngineClass getters or lack Attribute handle");
        ImGui::Separator();
        ImGui::Separator();
    }
}

//#endregion

// #region Main window - 'engine' args tab
void drawEngineAttributesTab(BeamClass@ actor, EngineClass@ engine)
{
    drawAttributesCommonHelp();
    
    ImGui::Columns(2);
    
    drawTableRow("getMinRPM", engine.getMinRPM());
    drawAttrInputRow(actor, ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM, "SHIFTDOWN_RPM");
    ImGui::Separator();
    drawTableRow("getMaxRPM", engine.getMaxRPM());
    drawAttrInputRow(actor, ACTORSIMATTR_ENGINE_SHIFTUP_RPM, "SHIFTUP_RPM");
    ImGui::Separator();
    drawTableRow("getEngineTorque", engine.getEngineTorque());
    drawAttrInputRow(actor, ACTORSIMATTR_ENGINE_TORQUE, "TORQUE");
    ImGui::Separator();
    drawTableRow("getDiffRatio", engine.getDiffRatio());
    drawAttrInputRow(actor, ACTORSIMATTR_ENGINE_DIFF_RATIO, "DIFF_RATIO");
    ImGui::Separator();    
    
    //float getGearRatio(int) const"
    //int getNumGears() const
    //int getNumGearsRanges() const
    ImGui::TextDisabled("gears (rev, neutral, "+engine.getNumGears()+" forward)");
    ImGui::NextColumn();
    for (int i = -1; i <= engine.getNumGears(); i++)
    {
        ImGui::NextColumn();
        ImGui::TextDisabled(">");
        ImGui::NextColumn();
        ImGui::Text(formatFloat(engine.getGearRatio(i), "", 0, 3));
    }
    ImGui::Columns(1);
}
//#endregion

//  #region Main window - 'engoption' args tab
void drawEngoptionAttributesTab(BeamClass@ actor, EngineClass@ engine)
{
    drawAttributesCommonHelp();
    
    ImGui::Columns(2);
    
    drawTableRow("getEngineInertia", engine.getEngineInertia());
    drawAttrInputRow(actor, ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA, "ENGINE_INERTIA");
    ImGui::Separator();
    drawTableRow("getEngineType", engine.getEngineType()); //uint8
    drawTableRow("isElectric", engine.isElectric()); // bool
    drawTableRow("hasAir", engine.hasAir()); // bool
    drawTableRow("hasTurbo", engine.hasTurbo()); // bool
    drawAttrInputRow(actor, ACTORSIMATTR_ENGOPTION_ENGINE_TYPE, "ENGINE_TYPE");
    ImGui::Separator();
    drawTableRow("getClutchForce", engine.getClutchForce());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE, "CLUTCH_FORCE");
    ImGui::Separator();
    drawTableRow("getShiftTime", engine.getShiftTime());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_SHIFT_TIME, "SHIFT_TIME");
    ImGui::Separator();
    drawTableRow("getClutchTime", engine.getClutchTime());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_CLUTCH_TIME, "CLUTCH_TIME");
    ImGui::Separator();
    drawTableRow("getPostShiftTime", engine.getPostShiftTime());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME, "POST_SHIFT_TIME");
    ImGui::Separator();
    
    drawTableRow("getStallRPM", engine.getStallRPM());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_STALL_RPM, "STALL_RPM");
    ImGui::Separator();
    drawTableRow("getIdleRPM", engine.getIdleRPM());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_IDLE_RPM, "IDLE_RPM");
    ImGui::Separator();
    drawTableRow("getMaxIdleMixture", engine.getMaxIdleMixture());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE, "MAX_IDLE_MIXTURE");
    ImGui::Separator();
    drawTableRow("getMinIdleMixture", engine.getMinIdleMixture());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE, "MIN_IDLE_MIXTURE");
    ImGui::Separator();
    drawTableRow("getBrakingTorque", engine.getBrakingTorque());
    drawAttrInputRow(actor,  ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE, "BRAKING_TORQUE");
    ImGui::Separator();
    
    ImGui::Columns(1);
}
// #endregion

// #region Main window - simulation state tab
void drawSimulationStateTab(EngineClass@ engine)
{
    ImGui::Columns(2);
    
    drawTableRow("getAcc", engine.getAcc());
    drawTableRowPlot("getClutch (0.0 - 1.0)", clutchBuf, 0.f, 1.f);
    drawTableRow("getCrankFactor", engine.getCrankFactor());
    drawTableRowPlot("getRPM (min - max)", rpmBuf, engine.getMinRPM(), engine.getMaxRPM());
    drawTableRow("getSmoke", engine.getSmoke());
    drawTableRowPlot("getTorque (0 - clutchforce)", clutchTorqueBuf, 0.f, engine.getClutchForce());
    drawTableRow("getTurboPSI", engine.getTurboPSI());
    drawTableRow("getAutoMode", engine.getAutoMode());//SimGearboxMode 
    drawTableRow("getGear", engine.getGear());
    drawTableRow("getGearRange", engine.getGearRange());
    drawTableRow("isRunning", engine.isRunning()); // bool
    drawTableRow("hasContact", engine.hasContact()); //bool
    drawTableRow("getAutoShift", engine.getAutoShift());   //autoswitch   
    drawTableRow("getCurEngineTorque", engine.getCurEngineTorque());
    drawTableRow("getInputShaftRPM", engine.getInputShaftRPM());
    drawTableRow("getDriveRatio", engine.getDriveRatio());
    drawTableRow("getEnginePower", engine.getEnginePower());
    drawTableRow("getTurboPower", engine.getTurboPower());
    drawTableRow("getIdleMixture", engine.getIdleMixture());
    drawTableRow("getPrimeMixture", engine.getPrimeMixture());
    drawTableRow("getAccToHoldRPM", engine.getAccToHoldRPM());
    
    ImGui::Columns(1);   
}
// #endregion

// #region Main window - clutch calc tab

void drawClutchCalcTab(EngineClass@ engine)
{
    
    ImGui::TextDisabled('// Replicating the clutch torque calculation');
    ImGui::SameLine();
    ImGui::Checkbox("cfgUseGlobalGearForThresholdCalc (visual only)", cfgUseGlobalGearForThresholdCalc);
    ImGui::Separator();
    
    ImGui::TextDisabled('float gearboxspinner = engine.getRPM() / engine.getGearRatio(engine.getGear());');
    float topGearRPM = engine.getMaxRPM() * engine.getGearRatio(engine.getNumGears() - 1);
    plotFloatBuf(calcGearboxSpinnerBuf, 0.f, topGearRPM*0.5f); 
    
    ImGui::TextDisabled('float clutchTorque = (gearboxspinner - engine.getWheelSpin()) * engine.getClutch() * engine.getClutchForce();');
    plotFloatBuf(calcClutchTorqueBuf, 0.f, 1000000);
    
    ImGui::TextDisabled('float forceThreshold = 1.5f * fmax(engine.getTorque(), engine.getEnginePower()) * fabs(engine.getGearRatio(1)) // BUG? should probably be 0=global ');
    plotFloatBuf(calcForceThresholdBuf, 0.f, 10*engine.getMaxRPM() * engine.getGearRatio(engine.getNumGears() - 1));
    
    ImGui::TextDisabled('float clutchTorqueClamped = fclamp(clutchTorque, -forceThreshold, forceThreshold);');
    plotFloatBuf(calcClutchTorqueClampedBuf, 100.f, topGearRPM*10); 
    
    ImGui::TextDisabled('float clutchTorqueFinal = clutchTorqueClamped * (1.f - fexp(-fabs(gearboxspinner - engine.getWheelSpin())));');
    plotFloatBuf(calcClutchTorqueFinalBuf, 100.f, topGearRPM*10);
}

// #endregion

// #region Main window
void drawEngineDiagUI(BeamClass@ actor, EngineClass@ engine)
{
    if (ImGui::BeginTabBar("engineDiagTabs"))
    {
        if (ImGui::BeginTabItem('engine args'))
        {
            drawEngineAttributesTab(actor, engine);
            ImGui::EndTabItem();
        }
        
        
        if (ImGui::BeginTabItem("engoption args"))
        {
            drawEngoptionAttributesTab(actor, engine);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem('state'))
        {
            drawSimulationStateTab(engine);
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("calc"))
        {
            drawClutchCalcTab(engine);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

// #endregion

float fmax(float a, float b)
{
    return (a > b) ? a : b;
}

float fabs(float a)
{
    return a > 0.f ? a : -a;
}

float fclamp(float val, float minv, float maxv)
{
    return val < minv ? minv : (val > maxv) ? maxv : val;
}

float fexp(float val)
{
    const float eConstant = 2.71828183;
    return pow(eConstant, val);
}

