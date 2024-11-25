// Test camera controls in global `game` object.
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

//#region yaw,pitch,roll
bool setRoll = false;
float rollAngle = 0.f;
bool setYaw = false;
float yawAngle = 0.f;
bool setPitch = false;
float pitchAngle = 0.f;

funcdef void YPRSETTERFUNC(float);

void drawYawpitchrollControl(YPRSETTERFUNC@ setterFunc, string axisName, bool&inout active, float&inout angle)
{
    ImGui::PushID(axisName);
    ImGui::Checkbox("game.setCamera"+axisName+"()", /*[inout]*/active);
    if (active)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("reset"))
        {
            angle = 0;
        }
        ImGui::SliderFloat("angle", /*[inout]*/angle, 0-360, 360);
        setterFunc(angle);
    }
    ImGui::PopID(); // axisName
}

//#endregion

//#region Position+Lookat
const float TERRAINSIZE=2000; // fake size... CBA to fetch actual size from terrn2
vector3 terrainPosMax = vector3(TERRAINSIZE, TERRAINSIZE, TERRAINSIZE);
bool setPosition=false;
bool posInit = false;
vector3 posValue = vector3(0,0,0);
bool setLookat=false;
bool lookatInit=false;
vector3 lookatValue  = vector3(0,0,0);
bool setDir = false;
bool dirInit=false;
vector3 dirValue = vector3(0,0,0);
vector3 resetDir() { return vector3(0,0,0); }


funcdef vector3 VEC3GETTER(void);
funcdef void VEC3SETTER(vector3&in);

void drawTerrainPosControl(VEC3SETTER@ setterFunc, VEC3GETTER@ resetFunc, string label, bool&inout init, bool&inout active, vector3&inout val, vector3 maxval)
{
    ImGui::PushID(label);
    if (!init)
    {
        val = resetFunc();
        init=true;
    }
    ImGui::Checkbox(label, active);
    if (active)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("reset"))
        {
            val = resetFunc();
        }
        ImGui::SliderFloat("X", val.x, 0, maxval.x);  
        ImGui::SliderFloat("Y (up)", val.y, 0, maxval.y);
        ImGui::SliderFloat("Z", val.z, 0, maxval.z);
        setterFunc(val);
    }
    ImGui::PopID(); // label
}
//#endregion

//#region EditorCam
bool editorCam = false;
vector2 edgeSize = vector2(75, 50);
vector2 edgeMotion = vector2(0,0);
vector3 editorCamPos = vector3(0,0,0);
vector3 editorCamDirection = vector3(0,0,0);
bool editorCamInit=false;
float editorCamElevation=175.f; // meters above ground
float editorCamSpeed = 2.5f;
void updateEditorCam()
{
    if (!editorCamInit)
    {
        editorCamPos = game.getPersonPosition();
        editorCamInit=true;
    }
    vector2 m = game.getMouseScreenPosition();
    vector2 s = game.getDisplaySize();
    // horizontal
if (m.x < edgeSize.x) { edgeMotion.x = -1; }   else if (m.x > (s.x - edgeSize.x)) { edgeMotion.x = 1; }   else { edgeMotion.x = 0; }
    // vertical
if (m.y < edgeSize.y) {edgeMotion.y = -1;} else if (m.y > (s.y - edgeSize.y)) {edgeMotion.y=1;} else {edgeMotion.y=0;}
    
    // apply motion
    editorCamPos.x += edgeMotion.x*editorCamSpeed;
    editorCamPos.z += edgeMotion.y*editorCamSpeed;
    // apply elevation
    editorCamPos.y = game.getTerrain().getHandle().getHeightAt(editorCamPos.x, editorCamPos.z) + editorCamElevation;
    //submit
    game.setCameraPosition(editorCamPos);
    // set orientation - pitch down 55degrees
    vector3 X_AXIS(1,0,0);
    radian pitchdown = degree(-55);
    game.setCameraOrientation( quaternion( pitchdown, X_AXIS));
}
void drawEditorCamUI()
{
    ImGui::PushID("editorCamUI");
    ImGui::Checkbox("EditorCam", editorCam);
    if (editorCam)
    {
        ImGui::Text("edgeMotion: X="+formatFloat(edgeMotion.x, '', 2, 1) + ", Y=" + formatFloat(edgeMotion.y, '', 2, 1));
        ImGui::SliderFloat("elevation", editorCamElevation, 1, 1000);
    }
    ImGui::PopID(); //editorCamUI
}
//#endregion

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Camera control test", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        //#region UI-Getters
        ImGui::TextDisabled("Getters:");
        ImGui::Text("game.getCameraPosition(): " + formatVector3(game.getCameraPosition(), 7, 2));
        ImGui::Text("game.getCameraDirection(): " + formatVector3(game.getCameraDirection(), 5, 2));
        //#endregion
        ImGui::Separator();
        //#region UI-Setters
        ImGui::TextDisabled("Setters:");
        drawYawpitchrollControl(YPRSETTERFUNC(game.setCameraYaw), "Yaw", setYaw, yawAngle);
        drawYawpitchrollControl(YPRSETTERFUNC(game.setCameraPitch), "Pitch", setPitch, pitchAngle);
        drawYawpitchrollControl(YPRSETTERFUNC(game.setCameraRoll), "Roll", setRoll, rollAngle);
        drawTerrainPosControl( VEC3SETTER(game.setCameraPosition),  VEC3GETTER(game.getCameraPosition), "game.setCameraPosition()", posInit, setPosition, posValue, terrainPosMax);    
        drawTerrainPosControl( VEC3SETTER(game.cameraLookAt),  VEC3GETTER(game.getPersonPosition), "game.cameraLookAt()", lookatInit, setLookat, lookatValue, terrainPosMax);
        drawTerrainPosControl( VEC3SETTER(game.setCameraDirection),  VEC3GETTER(resetDir), "game.setCameraDirection()", dirInit, setDir, dirValue, vector3(1,1,1));
        //#endregion
        ImGui::Separator();
        //#region Editor-cam test
        ImGui::TextDisabled("Top-down editor camera");
        ImGui::TextDisabled("with mouse-scrolling at screen edges");
        drawEditorCamUI();
        if (editorCam)
        {
            updateEditorCam();
        }
        //#endregion
        
        // End drawing window
        ImGui::End();
    }
}

//#region Helpers
string formatVector3(vector3 val, int total, int frac)
{
    return "X:" + formatFloat(val.x, "", total, frac)
    + " Y:" + formatFloat(val.y, "", total, frac)
    + " Z:" + formatFloat(val.z, "", total, frac);
}
//#endregion
