// TERRAIN OBJECT (*.odef) BROWSER prototype
// Lists .odef files from 'resources/meshes.zip' and sorts/displays/spawns/deletes them.  
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

const float FLT_MIN = 1.17549435e-38F;


//#region C O N F I G
int cfgGizmoNumSeg=10; // num segments for drawing circles on gizmos
color cfgGrabGizmoColBg = color(0,0,0,1);
color cfgGrabGizmoColFg = color(1,1,1,1);
float cfgGizmoScale = 1.f;
color cfgGizmoUiHoverColor = color(1,0.7,0.3,1);
color cfgGizmoSceneHoverColor = color(1,1,0,1);
int cfgGizmoHoverDistMax = 10; // screen-space distance in pixels
float cfgAxisGizmoPad = 0.7f; // world - meters
float cfgAxisGizmoLen = 1.2f; // world - meters
color cfgAxisGizmoColBg = color(0,0,0,1);
array<color> cfgAxisGizmoColFg = { color(1,0,0,1), color(0,1,0,1), color(0,0,1,1) };
float cfgHeightBiasRail = 0.4f;
float cfgHeightBiasMisc = 0.f;
color cfgGizmoGroundMarkerColBg = color(0,0,0,1);
color cfgGizmoGroundMarkerColFg = color(0,1,1,1);
color cfgObjSelectionHighlightCol = color(1,0,1,1);

void drawConfigPanel()
{
    ImGui::SliderFloat("gizmo scale", cfgGizmoScale, 0.1f, 10.f);
    ImGui::ColorEdit3("grab gizmo background", cfgGrabGizmoColBg);
    ImGui::ColorEdit3("grab gizmo foreground", cfgGrabGizmoColFg);
    ImGui::ColorEdit3("gizmo ui hover", cfgGizmoUiHoverColor);
    ImGui::ColorEdit3("gizmo scene hover", cfgGizmoSceneHoverColor);
    ImGui::SliderFloat("axis gizmo pad", cfgAxisGizmoPad, 0.1f, 5.f);
    ImGui::SliderFloat("axis gizmo len", cfgAxisGizmoLen, 0.1f, 5.f);
    ImGui::Separator();
    ImGui::InputFloat("height bias Rail", cfgHeightBiasRail);
    ImGui::InputFloat("height bias misc.", cfgHeightBiasMisc);
}
//#endregion


//#region  .ODEFs   L I S T I N G
array<dictionary>@ odefs = null; // fileinfo
string odefsError = "";
array<string> odefnamesAirport;
array<string> odefnamesAirportTaxiway;
array<string> odefnamesAirportRunway;
array<string> odefnamesRail;
array<string> odefnamesRailPlatform;
array<string> odefnamesRoad;
array<string> odefnamesRoadSign;
array<string> odefnamesOther;

void refreshODEFs()
{
    //FIXME: should reset all `odefnames*` arrays here
    @odefs = game.findResourceFileInfo("MeshesRG", "*.odef");
    if (@odefs == null)
    {
        odefsError="Could not load ODEFs, check RoR.log";
    }
    else
    {
        sortOdefs();
    }
}

void sortOdefs()
{
    for (uint iOdef=0; iOdef < odefs.length(); iOdef++)
    {
        string filename = string(odefs[iOdef]["basename"]);
        string odefname = filename.substr(0, filename.length() - 5);  //cut off ".odef" suffix
        
        // order matters! (i.e. 'sign-roadnarrows' must hit 'sign-', not 'road')
        if (odefname.findFirst("taxiway") >=0 || odefname.findFirst("taxient") >=0)
        {
            odefnamesAirportTaxiway.insertLast(odefname);            
        }
        else if (odefname.findFirst("heading") >=0 || odefname.findFirst("runway")>=0)
        {
            odefnamesAirportRunway.insertLast(odefname);            
        }
        else if (odefname.findFirst("airport") >=0 || odefname.findFirst("localizer") >= 0 )
        {            
            odefnamesAirport.insertLast(odefname);            
        }
        else if (odefname.findFirst("railplatform")>=0)
        {
            odefnamesRailPlatform.insertLast(odefname);
        }
        else if (odefname.findFirst("rail")>=0)
        {
            odefnamesRail.insertLast(odefname);
        }   
        else if (odefname.findFirst("sign-")>=0)
        {
            odefnamesRoadSign.insertLast(odefname);
        }
        else if (odefname.findFirst("road")>=0)
        {
            odefnamesRoad.insertLast(odefname);
        }     
        else
        {
            odefnamesOther.insertLast(odefname);
        }
    }
}

void drawOdefTree()
{
    if (ImGui::TreeNode("Airport ("+odefnamesAirport.length()+")"))
    {
        drawOdefNames(odefnamesAirport, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Airport Taxiway ("+odefnamesAirportTaxiway.length()+")"))
    {
        drawOdefNames(odefnamesAirportTaxiway, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Airport Runway ("+odefnamesAirportRunway.length()+")"))
    {
        drawOdefNames(odefnamesAirportRunway, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Rail ("+odefnamesRail.length()+")"))
    {
        drawOdefNames(odefnamesRail, cfgHeightBiasRail);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Rail Platform ("+odefnamesRailPlatform.length()+")"))
    {
        drawOdefNames(odefnamesRailPlatform, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Road ("+odefnamesRoad.length()+")"))
    {
        drawOdefNames(odefnamesRoad, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Road Sign ("+odefnamesRoadSign.length()+")"))
    {
        drawOdefNames(odefnamesRoadSign, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Other ("+odefnamesOther.length()+")"))
    {
        drawOdefNames(odefnamesOther, cfgHeightBiasMisc);
        ImGui::TreePop();
    }
}

void drawOdefNames(array<string>@ arr, float hBias)
{
    // helper for `drawOdefTree()` - draws odef names and spawn button
    // --------------------
    for (uint i = 0; i<arr.length(); i++) 
    {
        ImGui::PushID(i);
        
        ImGui::BulletText(arr[i]);
        ImGui::SameLine();
        if (ImGui::SmallButton("Spawn!"))
        {
            spawnObjectAtRorbot(arr[i], hBias);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Manual..."))
        {
            manualSpawnPanelOdefName = arr[i];
            manualSpawnPanelHBias = hBias;
        }
        
        ImGui::PopID(); // i
    }
}
//#endregion


//#region O B J E C T S
class ObjInstanceInfo
{ 
    string name;
    string odefName;
    vector3 pos; 
    vector3 rot;
    bool uiHovered=false;
    bool sceneHovered=false;
    int sceneHoveredAxis = -1; // -1 is none, 0/1/2 are x/y/z
    bool inSelection=false;
ObjInstanceInfo(string _name, string oName, vector3 _pos, vector3 _rot) {name=_name; odefName=oName; pos=_pos; rot=_rot;}
};
dictionary objInstances; // instanceName ->ObjInstanceInfo
uint spawnedObjSequentialNum=0;
string manualSpawnPanelOdefName = "";

string composeUniqueInstanceName(string odefname)
{
    // To avoid conflict with leftover objects created before, we include the "script unit ID" (aka NID) to the instance name.
    // Format: s#c#n# (S=sequential number, C=instance count, N=NID)
    // -------------------------------
    return odefname +"~s"+(spawnedObjSequentialNum++)+ "c" + objInstances.getKeys().length() + "n"+thisScript;
}

void spawnObjectAtRorbot(string odefname, float hBias)
{
    vector3 objectRot = vector3(0,-game.getPersonRotation().valueDegrees(),0);
    vector3 objectPos = game.getPersonPosition() + vector3(0,hBias,0);
    spawnObjectManually(odefname, objectPos, objectRot);
}

void spawnObjectManually(string odefname, vector3 objectPos, vector3 objectRot)
{
    string instanceName = composeUniqueInstanceName(odefname);
    game.spawnObject(odefname, instanceName, objectPos, objectRot, "", false);
    ObjInstanceInfo@ instInfo = ObjInstanceInfo (instanceName, odefname, objectPos, objectRot);
    @objInstances[instanceName] = @instInfo;
}

void drawSpawnedObjectsPanel()
{
    ImGui::PushID("spawnedObjs");    
    array<string>@ spawnedObjectInstanceIds = objInstances.getKeys();
    
    ImGui::TextDisabled("spawned objects ("+spawnedObjectInstanceIds.length()+"): ");
    for (uint iInst = 0; iInst < spawnedObjectInstanceIds.length(); iInst++)
    {
        ImGui::PushID(iInst);        
        ObjInstanceInfo@ instInfo = cast<ObjInstanceInfo>(objInstances[spawnedObjectInstanceIds[iInst]]);
        
        //selection checkbox
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, vector2(2,0));
        ImGui::Checkbox("Sel", instInfo.inSelection);
        ImGui::PopStyleVar();
        ImGui::SameLine();
        
        // instance name
        if (instInfo.uiHovered) ImGui::PushStyleColor(ImGuiCol_Text, cfgGizmoUiHoverColor);
        if (instInfo.sceneHovered && instInfo.sceneHoveredAxis == -1) ImGui::PushStyleColor(ImGuiCol_Text,  cfgGizmoGroundMarkerColFg);
        if (instInfo.sceneHovered && instInfo.sceneHoveredAxis == 0) ImGui::PushStyleColor(ImGuiCol_Text, cfgAxisGizmoColFg[0]);
        if (instInfo.sceneHovered && instInfo.sceneHoveredAxis == 1) ImGui::PushStyleColor(ImGuiCol_Text, cfgAxisGizmoColFg[1]);
        if (instInfo.sceneHovered && instInfo.sceneHoveredAxis == 2) ImGui::PushStyleColor(ImGuiCol_Text, cfgAxisGizmoColFg[2]);
        if (instInfo.inSelection) ImGui::PushStyleColor(ImGuiCol_Text, cfgObjSelectionHighlightCol);
        ImGui::BulletText(instInfo.name);
        if (instInfo.uiHovered) ImGui::PopStyleColor(1); //cfgGizmoUiHoverColor
        if (instInfo.sceneHovered)  ImGui::PopStyleColor(1); // cfgGizmoSceneHoverColor || cfgAxisGizmoColF
        if (instInfo.inSelection) ImGui::PopStyleColor(1); // cfgObjSelectionHighlightCol
        
        bool bulletHovered = ImGui::IsItemHovered();
        ImGui::SameLine();
        if (ImGui::SmallButton("Destroy!"))
        {
            game.destroyObject(instInfo.name);
            objInstances.delete(instInfo.name);
        }
        bool dBtnHovered = ImGui::IsItemHovered();
        
        instInfo.uiHovered = bulletHovered || dBtnHovered;
        
        ImGui::PopID(); //iInst
    }    
    ImGui::PopID(); // spawnedObjs
}
//#endregion



//#region G I Z M O S
int gizmoClosestDist = INT_MAX;
string gizmoClosestObj = "";
int gizmoClosestObjAxis = -1; // -1=none, 0=x, 1=y, 2=z
string gizmoFocusedObj = ""; // Mouse is down - grab is active
int gizmoFocusedObjAxis = -1;
vector2 gizmoPrevMouseScreenPos(0,0);



void drawSingleObjectGizmos(string instName, ObjInstanceInfo@ instInfo)
{
    ImDrawList @drawlist = imgui_utils::ImGetDummyFullscreenWindow("ODEF browser gizmos");
    bool grabHovered = (instInfo.inSelection || instInfo.uiHovered || (instInfo.sceneHovered && instInfo.sceneHoveredAxis == -1));
    
    color hColor = color(0,0,0,1);
if (instInfo.uiHovered) { hColor = cfgGizmoUiHoverColor ; }
else if (instInfo.inSelection) {hColor = cfgObjSelectionHighlightCol; }
else { hColor =  cfgGizmoSceneHoverColor; }
    
    
    // draw object origin visualization - fake knob, not actually hoverable (empty param `instName` disables hover)
    if (drawGizmoMouseKnob(drawlist, "", instInfo.pos, -1, cfgGrabGizmoColFg, cfgGrabGizmoColBg, /*hovered:*/false, hColor))
    {
        // object is visible on screen ~ draw axes gizmos
        drawAxisGizmo(drawlist, instName, instInfo, 0);
        drawAxisGizmo(drawlist, instName, instInfo, 1);
        drawAxisGizmo(drawlist, instName, instInfo, 2);
        
        // draw elevation marker  = actual grab knob
        vector3 groundPoint(instInfo.pos.x, game.getGroundHeight(instInfo.pos), instInfo.pos.z);
        drawGizmoMouseKnob(drawlist, instName, groundPoint, -1, cfgGizmoGroundMarkerColFg, cfgGizmoGroundMarkerColBg, grabHovered, hColor);
    }
}



bool drawGizmoMouseKnob(ImDrawList @drawlist, string instName, vector3 wPos, int axis, color colFg, color colBg, bool hovered, color colHover) // Helper for multiple gizmo funcs
{
    vector2 originScreenPos;
    bool onScreen = game.getScreenPosFromWorldPos(wPos, /*out:*/originScreenPos);
    if (onScreen)
    {
        // background filled circle with a circle within
        drawlist.AddCircleFilled(originScreenPos, 8*cfgGizmoScale, colFg, cfgGizmoNumSeg);
        drawlist.AddCircle(originScreenPos, 5*cfgGizmoScale, colBg, cfgGizmoNumSeg, 2*cfgGizmoScale);
        
        if (instName != "") // empty param `instName` disables hover
        {
            
            // hover highlight
            if (hovered) 
            {
                drawlist.AddCircle(originScreenPos, 10*cfgGizmoScale, colHover, cfgGizmoNumSeg, 3*cfgGizmoScale);
            }
            
            // seeking hovered element
            
            int dist = getMouseShortestDistance(game.getMouseScreenPosition(), originScreenPos);
            if (dist < gizmoClosestDist)
            {
                gizmoClosestDist = dist;
                gizmoClosestObj = instName;
                gizmoClosestObjAxis = axis;
            }
        }
    }
    return onScreen;
}

vector3 getAxisGizmoPoint(vector3 base, int axis, float offset) // Helper for `drawAxisGizmo()`
{
    return base + vector3( (axis==0?offset:0.f), (axis==1?offset:0.f), (axis==2?offset:0.f) );
}

void drawAxisGizmo(ImDrawList @drawlist, string instName, ObjInstanceInfo@ instInfo, int axis) // Helper for `drawSingleObjectGizmos()`
{
    vector3 axStartWorldPos = getAxisGizmoPoint(instInfo.pos, axis, cfgAxisGizmoPad);
    vector3 axEndWorldPos = getAxisGizmoPoint(axStartWorldPos, axis, cfgAxisGizmoLen);
    vector2 axStartScreenPos, axEndScreenPos; 
    if (game.getScreenPosFromWorldPos(axStartWorldPos, /*[out]*/axStartScreenPos)
    && game.getScreenPosFromWorldPos(axEndWorldPos, /*[out]*/axEndScreenPos))
    {
        drawlist.AddLine(axStartScreenPos, axEndScreenPos, cfgAxisGizmoColBg, 8*cfgGizmoScale);
        drawlist.AddLine(axStartScreenPos, axEndScreenPos, cfgAxisGizmoColFg[axis], 4*cfgGizmoScale);
        
        bool axKnobHovered = (instInfo.uiHovered || (instInfo.sceneHovered && (instInfo.sceneHoveredAxis == axis || instInfo.sceneHoveredAxis == -1)));
        vector3 axKnobPos = axStartWorldPos + ((axEndWorldPos - axStartWorldPos)/2);
        color axHovColor = (axKnobHovered && instInfo.uiHovered) ? cfgGizmoUiHoverColor : cfgGizmoSceneHoverColor;
        drawGizmoMouseKnob(drawlist, instName, axKnobPos, axis,  cfgAxisGizmoColBg, cfgAxisGizmoColFg[axis], axKnobHovered, axHovColor);
    }
}

void drawAllObjectsGizmos()
{
    //  reset vars for mouse hover seeking
    if (gizmoFocusedObj == "")
    {
        gizmoClosestDist = INT_MAX;
        gizmoClosestObj = "";
    }
    
    // Looop over all gizmos
    array<string>@ spawnedObjectInstanceIds = objInstances.getKeys();
    for (uint iInst = 0; iInst < spawnedObjectInstanceIds.length(); iInst++)
    {
        string instName = spawnedObjectInstanceIds[iInst];
        ObjInstanceInfo@ instInfo = null;
        if (objInstances.get(instName, @instInfo))
        {
            
            drawSingleObjectGizmos(instName, instInfo);       
            instInfo.sceneHovered=false;    
        }
    }
}

void updateInputEvents()
{
    
    // evaluate mouse hover
    // ------
    if (gizmoFocusedObj == "")
    {
        if (gizmoClosestDist <= (cfgGizmoHoverDistMax * cfgGizmoScale))
        {
            ObjInstanceInfo@ instInfo = null;
            if (objInstances.get(gizmoClosestObj, @instInfo))
            {          
                instInfo.sceneHovered=true;
                instInfo.sceneHoveredAxis = gizmoClosestObjAxis;
                if (ImGui::IsMouseDown(0))
                {
                    gizmoFocusedObj = gizmoClosestObj;
                    gizmoFocusedObjAxis = gizmoClosestObjAxis;
                }
            }
        }
    }
    else
    {
        ObjInstanceInfo@ instInfo = null;
        if (ImGui::IsMouseDown(0) && objInstances.get(gizmoFocusedObj, @instInfo))
        {
            if ( gizmoFocusedObjAxis == -1)
            {
                vector3 tMouse;
                if (game.getMousePositionOnTerrain(/*[out]*/ tMouse)  )
                {
                    
                    instInfo.pos = vector3(tMouse.x, instInfo.pos.y, tMouse.z);
                    game.moveObjectVisuals(gizmoFocusedObj, instInfo.pos);
                }
            }
            else if (gizmoFocusedObjAxis>=0 && gizmoFocusedObjAxis <= 2)
            {
                vector3 axStartWorldPos = getAxisGizmoPoint(instInfo.pos, gizmoFocusedObjAxis, cfgAxisGizmoPad);
                vector3 axEndWorldPos = getAxisGizmoPoint(axStartWorldPos, gizmoFocusedObjAxis, cfgAxisGizmoLen);
                vector2 axStartScreenPos, axEndScreenPos; 
                if (game.getScreenPosFromWorldPos(axStartWorldPos, /*[out]*/axStartScreenPos)
                && game.getScreenPosFromWorldPos(axEndWorldPos, /*[out]*/axEndScreenPos))
                {
                    vector2 axScreenVec = axEndScreenPos - axStartScreenPos;
                    vector2 mScreenVec = game.getMouseScreenPosition() - gizmoPrevMouseScreenPos;
                    float axRatioXtoY = fabs(axScreenVec.x) / fabs(axScreenVec.y);
                    // watch out for dividing by zero
                    float axDistX = (axScreenVec.x == 0.f) ? 0.f : (mScreenVec.x / axScreenVec.x) * axRatioXtoY;
                    float axDistY = (axScreenVec.y == 0.f) ? 0.f : (mScreenVec.y / axScreenVec.y) * (1.f - axRatioXtoY);
                    float axDist = (axDistX + axDistY);
                    vector3 translation; 
                    setAxisVal(translation, gizmoFocusedObjAxis, axDist*cfgAxisGizmoLen);
                    instInfo.pos += translation;
                    game.moveObjectVisuals(gizmoFocusedObj, instInfo.pos);
                }
            }
        }
        else
        {
            if (objInstances.get(gizmoFocusedObj, @instInfo))
            {
                // respawn the object to update collisions
                game.destroyObject(instInfo.name);
                game.spawnObject(instInfo.odefName, instInfo.name, instInfo.pos, instInfo.rot, "", false);
            }
            gizmoFocusedObj = "";
        }
    }
    
    
    gizmoPrevMouseScreenPos = game.getMouseScreenPosition();
    
}
//#endregion


//#region  U I   T O O L S

float manualSpawnPanelHBias = 0.f;
vector3 manualSpawnPanelObjPos = vector3(0,0,0);
vector3 manualSpawnPanelObjRot = vector3(0,0,0);

void drawManualSpawnPanel()
{
    ImGui::PushID("manualSpawn");
    
    ImGui::TextDisabled("Specify parameters by hand");
    
    ImGui::InputText("ODEF", manualSpawnPanelOdefName);
    if (manualSpawnPanelOdefName != "")
    { 
        ImGui::SameLine();
        if (ImGui::SmallButton("clear"))
        {
            manualSpawnPanelOdefName = "";
        }
    }
    ImGui::Text("Position (remember Y is up):");  ImGui::SameLine();
    if (ImGui::SmallButton("<-- Fill from character"))
    {
        manualSpawnPanelObjPos = game.getPersonPosition() + vector3(0,manualSpawnPanelHBias,0);
    }
    ImGui::Dummy(vector2(25, 0)); ImGui::SameLine();
    ImGui::SetNextItemWidth(75.f);   ImGui::InputFloat("X", manualSpawnPanelObjPos.x); ImGui::SameLine();
    ImGui::SetNextItemWidth(75.f);   ImGui::InputFloat("Y", manualSpawnPanelObjPos.y); ImGui::SameLine();
    ImGui::SetNextItemWidth(75.f);   ImGui::InputFloat("Z", manualSpawnPanelObjPos.z);
    
    
    ImGui::Text("Rotation: (degrees):");  ImGui::SameLine(); 
    if (ImGui::SmallButton("<-- Fill from character"))
    {
        manualSpawnPanelObjRot = vector3(0,-game.getPersonRotation().valueDegrees(),0);
    }
    ImGui::Dummy(vector2(25, 0)); ImGui::SameLine();
    ImGui::SetNextItemWidth(75.f);   ImGui::InputFloat("X", manualSpawnPanelObjRot.x); ImGui::SameLine();
    ImGui::SetNextItemWidth(75.f);   ImGui::InputFloat("Y", manualSpawnPanelObjRot.y); ImGui::SameLine();
    ImGui::SetNextItemWidth(75.f);   ImGui::InputFloat("Z", manualSpawnPanelObjRot.z); 
    
    if ( manualSpawnPanelOdefName != "" && ImGui::Button("Spawn manually!"))
    {
        spawnObjectManually(manualSpawnPanelOdefName, manualSpawnPanelObjPos, manualSpawnPanelObjRot);
    }
    
    ImGui::PopID(); // "manualSpawn"
}

void drawSelectionUtilsPanel()
{
    // Tools that affect current selection: alignment, spacing etc...
    // --------------------------------------------------------------
    
    // gather statistics
    int statNumObjs = 0;
    vector3 statExtentMin (FLT_MAX, FLT_MAX, FLT_MAX);
    
    
    
    vector3 statExtentMax (FLT_MIN, FLT_MIN, FLT_MIN);
    
    array<string>@ objInstancesKeys = objInstances.getKeys();
    for (uint iKey = 0; iKey < objInstancesKeys.length(); iKey++)
    {
        string iInst = objInstancesKeys[iKey];
        ObjInstanceInfo@ instInfo = cast<ObjInstanceInfo@>(objInstances[iInst]);
        if (instInfo.inSelection)
        {
            //          statNumObjects++;
            
            statExtentMin.x = fmin(instInfo.pos.x, statExtentMin.x);
            statExtentMin.y = fmin(instInfo.pos.y, statExtentMin.y);
            statExtentMin.z = fmin(instInfo.pos.z, statExtentMin.z);
            
            statExtentMin.x = fmax(instInfo.pos.x, statExtentMax.x);
            statExtentMin.y = fmax(instInfo.pos.y, statExtentMax.y);
            statExtentMin.x = fmax(instInfo.pos.z, statExtentMax.z);            
        }
    }
}
//#endregion


//#region G A M E    C A L L B A C K S

void frameStep(float dt)
{
    drawAllObjectsGizmos();
    updateInputEvents();
    if ( (@odefs == null && odefsError == ""))
    {
        refreshODEFs();
    }
    
    if (ImGui::Begin("ODEF browser", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        
        drawMainPanel();
        
        ImGui::End();
    }
}

void drawMainPanel()
{
    
    
    ImGui::TextDisabled("T H E   T O O L");
    if (ImGui::CollapsingHeader("Status (diag)"))
    {
        ImGui::Text("Focused obj: " + gizmoFocusedObj);
        ImGui::Text("Focused obj axis: " + gizmoFocusedObjAxis);
        ImGui::Text("IsMouseDown(0 = LMB): "+ImGui::IsMouseDown(0));
        ImGui::Text("IsMouseDown(1 = RMB): "+ImGui::IsMouseDown(1));
        ImGui::Text("Is left ALT key down: " + inputs.isKeyDown(KC_LALT));
        ImGui::Text("Is right ALT key down: " + inputs.isKeyDown(KC_RALT));
        ImGui::Text("ODEFs loading error: " + odefsError);
    }
    if (ImGui::CollapsingHeader("Config"))
    {
        drawConfigPanel();
    }
    
    ImGui::Separator();
    ImGui::TextDisabled("O B J   D E F S");
    
    if (ImGui::CollapsingHeader("Full ODEF listing"))
    {
        if (ImGui::Button("Refresh"))
        {
            ImGui::Button("refresh ODEFs");
        }
        for (uint iOdef=0; iOdef < odefs.length(); iOdef++)
        {
            ImGui::BulletText(string(odefs[iOdef]["basename"]));
        }
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextDisabled("  Total loaded: "+odefs.length());
    }
    
    if (ImGui::CollapsingHeader("ODEFs by category"))
    {
        drawOdefTree();
    }
    
    ImGui::Separator();
    ImGui::TextDisabled("O B J E C T S");
    
    
    if (ImGui::CollapsingHeader("Manual spawn"))
    {
        drawManualSpawnPanel();
    }
    drawSpawnedObjectsPanel();
    
    
}
//#endregion


//#region    H E L P E R S


int clamp(int minval, int val, int maxval)
{
    return max(minval, min(val, maxval));
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

float fmax(float a, float b)
{
    return (a > b) ? a : b;
}

float fmin(float a, float b)
{
    return (a < b) ? a : b;
}

int abs(int a)
{
    return (a < 0) ? -a : a;
}

float fabs(float f)
{
    return (f<0.f)?-f:f;
}



int getMouseShortestDistance(vector2 mouse, vector2 target)
{
    int dx = abs(int(mouse.x) - int(target.x));
    int dy = abs(int(mouse.y) - int(target.y));
    return max(dx, dy);
}

void setAxisVal(vector3&inout vec, int axis, float val) {
    switch(axis) {
        case 0: vec.x=val; break;
        case 1: vec.y=val; break;
        case 2: vec.z=val; break;
        default: break;
    }
}

//#endregion
