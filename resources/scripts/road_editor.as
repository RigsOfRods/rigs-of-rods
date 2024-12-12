
//    ---------------------------------------------------------------------------
//                  Project Rigs of Rods (www.rigsofrods.org)
//                            
//                             PROCEDURAL ROAD EDITOR                                
//    
//    Procedural roads are static tracks generated from splines.
//    Splines can be defined in .tobj files which come with terrain (.terrn2),
//    but can also be created programmatically.
//    
//    To run, open in-game console (Hotkey ~) and say 'loadscript road_editor.as'
//    
//    Procedural roads .tobj documentation:
//    https://docs.rigsofrods.org/terrain-creation/old-terrn-subsystem/#procedural-roads
//    
//    Scripting documentation:
//    https://developer.rigsofrods.org/d4/d07/group___script2_game.html
//     
//    ---------------------------------------------------------------------------


#include "imgui_utils.as"

// Game state
CVarClass@ cvar_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
CVarClass@ cvar_sistate = console.cVarFind("sim_state"); // 0=off, 1=running, 2=paused, 3=terrain editor, see SimState in Application.h
CVarClass@ cvar_mp_state = console.cVarFind("mp_state"); // 0=disabled, 1=connecting, 2=connected, see MpState in Application.h

// Main window
imgui_utils::CloseWindowPrompt closeBtnHandler;






//#region Game callbacks

void main()
{
    //    Script setup function - invoked by game once when script is loaded.
    //    ---------------------------------------------------------------------------
    log("Road editor loaded! Enter terrain editing mode (hotkey: " + inputs.getEventCommandTrimmed(EV_COMMON_TOGGLE_TERRAIN_EDITOR) + ") to use it.");
}


void frameStep(float dt)
{
    //    Script update function - invoked once every rendered frame, with elapsed time (delta time, in seconds) as parameter.
    //    ---------------------------------------------------------------------------
    
    if (cvar_app_state.getInt() == 2 // simulation
    && cvar_sistate.getInt() == 3 // terrain editor mode
    && cvar_mp_state.getInt() == 0) // singleplayer
    {
        if (editorCam)
        {
            updateEditorCam();
        }
        drawMainWindow();
        
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        if (roads.getNumObjects() == 0)
        {
            return; // nothing more to do
        }
        else
        {
            ProceduralObjectClass@ obj = roads.getObject(selected_road);
            if (obj.getNumPoints() == 0)
            {
                return; // nothing more to do
            }
        }
        visualizeRoad();
        updateMeshRebuildInputEvents();
        updateEditRoadInputEvents();
    }
}

//#endregion

//#region Main window
void drawMainWindow()
{
    int flags = ImGuiWindowFlags_NoCollapse; 
    if (ImGui::Begin("Road editor", closeBtnHandler.windowOpen, flags))
    {
        
        closeBtnHandler.draw();  // draw the "terminate script?" proompt?
        ImGui::Dummy(vector2(200.f, 1.f)); // force width
        
        int barflags = 0;
        if (ImGui::BeginTabBar("Road editor tabs", barflags))
        {
            if (ImGui::BeginTabItem("Manage roads"))
            {
                drawRoadListPanel();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Edit road"))
            {
                drawRoadEditPanel();                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Mesh rebuild"))
            {
                
                drawMeshRebuildPanel();
                
                ImGui::EndTabItem();
            }    
            
            if (ImGui::BeginTabItem("AI Waypoints"))
            {
                drawAiWaypointsPanel();                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Camera"))
            {
                drawEditorCamUI();
                
                ImGui::EndTabItem();
            }		
            
            ImGui::EndTabBar();
        }	
        
        // End window
        ImGui::End();
    }
}
//#endregion

//#region Edit road tab
int selected_point = 0; // use `setSelectedPoint()`
int hovered_point = 0;
float selected_point_elevation = 0.f;
float hovered_point_elevation = 0.f;  
bool selected_point_auto_goto = false;

void setSelectedPoint(int point)
{
    if (point != selected_point)
    {
        selected_point = point;
        if (selected_point_auto_goto)
        {
            TerrainClass@ terrain = game.getTerrain();
            ProceduralManagerClass@ proc_mgr = terrain.getProceduralManager();
            ProceduralObjectClass@ proc_obj = proc_mgr.getObject(selected_road);
            
            goToPoint(proc_obj, selected_point);
        }
    }
}

void goToPoint(ProceduralObjectClass@ obj, int point_idx)
{
    ProceduralPointClass@ point = obj.getPoint(point_idx);
    game.setPersonPosition(point.position);
    game.setPersonRotation(point.rotation.getYaw()); // I have no idea if this is correct.
}

void drawRoadEditPanel()
{
    ImGui::PushID("road edit box");
    
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    if (roads.getNumObjects() > 0)
    {
        ProceduralObjectClass@ obj = roads.getObject(selected_road);
        ImGui::Text("Road: " + obj.getName());
        ImGui::Text("Total points: " + obj.getNumPoints());
        if (obj.getNumPoints() > 0)
        {
            ImGui::TextDisabled("Use slider or left-click road point to select it.");
            ImGui::TextDisabled("Press hotkey '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_SET_POS) + "' to move the selected point to mouse position");
            int slider_val = selected_point;
            if (ImGui::SliderInt("", slider_val, 0, obj.getNumPoints() - 1))
            {
                setSelectedPoint(slider_val);
            }
            if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_GOTO)
            || ImGui::Button("Go to point (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_GOTO) + "')"))
            {
                goToPoint(obj, selected_point);
            }
            ImGui::SameLine();            
            ImGui::Checkbox("Auto", selected_point_auto_goto);
        }
        
        ImGui::TextDisabled("Add new road point at the character position and link it to selected road point (if any).");
        if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_INSERT)
        || ImGui::Button("Insert new point (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_INSERT) + "')"))
        {
            addPointToCurrentRoad(game.getPersonPosition());
        }
        
        if (obj.getNumPoints() > 0)
        {
            if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_DELETE)
            || ImGui::Button("Remove selected point (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_DELETE) + "')"))
            {
                deletePointFromCurrentRoad(selected_point);
            }
        }
        
        if (ImGui::CollapsingHeader("Point properties"))
        {
            drawPointPropertiesPanel(obj);
        }              
        
        
    }
    ImGui::PopID(); // "road edit box"
}

void drawRoadTypeRadioBtn(ProceduralPointClass@ point, string&in label, RoadType roadType, int pillarType)
{
    bool selected =( point.type == roadType && point.pillar_type == pillarType );
    
    if (ImGui::RadioButton(label, selected))
    {
        point.type = roadType;
        point.pillar_type = pillarType;
    }
}

void drawPointPropertiesPanel(ProceduralObjectClass@ obj)
{
    ImGui::PushID("point properties");
    if (obj.getNumPoints() > 0)
    {
        ProceduralPointClass@ point = obj.getPoint(selected_point);
        
        ImGui::SetNextItemWidth(130.f);
        ImGui::InputFloat("Elevation (meters)", point.position.y);
        
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Width (meters)", point.width);   
        
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Border width (meters)", point.border_width);  
        
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Border height (meters)", point.border_height);
        
        ImGui::Text("Type:");
        ImGui::SameLine();
        ImGui::TextDisabled("DBG type:"+point.type+", pillartype:"+point.pillar_type);
        // Types supported by TOBJ format
        drawRoadTypeRadioBtn(point, "(automatic)" ,ROAD_AUTOMATIC, 0);
        drawRoadTypeRadioBtn(point, "flat (no border)",  ROAD_FLAT, 0);
        drawRoadTypeRadioBtn(point, "left (border on left)",  ROAD_LEFT, 0);
        drawRoadTypeRadioBtn(point, "right (border on right)", ROAD_RIGHT, 0);
        drawRoadTypeRadioBtn(point, "both (with borders)", ROAD_BOTH, 0);
        drawRoadTypeRadioBtn(point, "bridge (with pillars)",  ROAD_BRIDGE , 1);
        drawRoadTypeRadioBtn(point, "bridge_no_pillars",  ROAD_BRIDGE , 0);
        drawRoadTypeRadioBtn(point, "monorail (with pillars)", ROAD_MONORAIL , 2);
        drawRoadTypeRadioBtn(point, "monorail2 (no pillars)", ROAD_MONORAIL , 0);
        // End of types supported by TOBJ format
    }
    else
    {
        ImGui::Text("Road has no points - nothing to edit!");
    }        
    
    ImGui::PopID(); //"point properties"
}

void addPointToCurrentRoad(vector3 pos)
{
    ProceduralObjectClass@ obj = game.getTerrain().getHandle().getProceduralManager().getHandle().getObject(selected_road);
    
    ProceduralPointClass@ point = ProceduralPointClass();
    if (obj.getNumPoints() > 0)
    {
        ProceduralPointClass@ template = obj.getPoint(selected_point);
        point.type = template.type;
        point.pillar_type = template.pillar_type;
        point.width = template.width;
        point.border_width = template.border_width;
        point.border_height = template.border_height;
    }
    point.position = pos;            
    
    if (obj.getNumPoints() == 0 || selected_point == obj.getNumPoints() - 1)
    {
        obj.addPoint(point);
        selected_point = obj.getNumPoints() - 1;
    }
    else
    {
        obj.insertPoint(selected_point, point);
    }
}  

void deletePointFromCurrentRoad(int pos)
{
    ProceduralObjectClass@ obj = game.getTerrain().getHandle().getProceduralManager().getHandle().getObject(selected_road);
    
    obj.deletePoint(pos);
    if (pos >= obj.getNumPoints())
    {
        if (obj.getNumPoints() > 0)
        setSelectedPoint(obj.getNumPoints() - 1);
        else
        selected_point = 0;
    }
}

void updateEditRoadInputEvents()
{
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    ProceduralObjectClass@ obj = roads.getObject(selected_road);    
    
    vector3 mouse_tpos;
    if (inputs.getEventBoolValue(EV_ROAD_EDITOR_POINT_SET_POS)
    && game.getMousePositionOnTerrain(mouse_tpos))
    {
        // Y is 'up'.
        ProceduralPointClass@ pp = obj.getPoint(selected_point);
        //vector3 old_pos = pp.position;
        vector3 new_pos = mouse_tpos + vector3(0,selected_point_elevation,0);
        //game.log("Updating road '"+obj.getName()+"', point '"+selected_point+"' position to X:"+new_pos.x+" Y:"+new_pos.y+" Z:"+new_pos.z+" (previously X:"+old_pos.x+" Y:"+old_pos.y+" Z:"+old_pos.z+")");
        pp.position = new_pos;
    }
}

//#endregion

//#region Mesh rebuild tab
float global_extra_point_elevation = 0; // Added to all points
float global_min_point_elevation = 0.3; // Added to lower points

void drawMeshRebuildPanel()
{
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    ProceduralObjectClass@ obj = roads.getObject(selected_road);
    if (@obj == null)
    {
        ImGui::Text("No road selected!");
        return;
    }
    
    ImGui::PushID("mesh rebuild");
    
    
    if (obj.getNumPoints() > 0)
    {
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Common minimum point elevation (meters)", global_min_point_elevation);
        
        ImGui::SetNextItemWidth(100.f);
        ImGui::InputFloat("Common extra point elevation (meters)", global_extra_point_elevation);            
        
        ImGui::SetNextItemWidth(120.f);
        
    // create temporary varible because `smoothing_num_splits` is a property {get;set;}
        int objSmoothingNumSplits = obj.smoothing_num_splits;
        if (ImGui::InputInt("Num smoothing splits (0=off)", objSmoothingNumSplits))
        {
            obj.smoothing_num_splits = objSmoothingNumSplits;
        }
    // create temporary varible because `collision_enabled` is a property {get;set;}
        bool objCollisionEnabled = obj.collision_enabled;
        if(ImGui::Checkbox("Collision enabled", objCollisionEnabled))
        {
            obj.collision_enabled = objCollisionEnabled;
        }
        
        // NOTE: hotkey is processed in `updateInputEvents()` to be independent of the UI
        if (ImGui::Button("Rebuild road mesh (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_REBUILD_MESH) + "')"))
        {
            rebuildMesh(obj);
        } 
    }
    else
    {
        ImGui::Text("Road has no points - nothing to generate!");
    }
    
    ImGui::PopID(); //"mesh rebuild"
}

void rebuildMesh(ProceduralObjectClass@ obj)
{
    ProceduralManagerClass@ roads = game.getTerrain().getHandle().getProceduralManager();
    
    recalculatePointElevations(obj);
    recalculatePointRotations(obj);
    roads.rebuildObjectMesh(obj);
}

void recalculatePointElevations(ProceduralObjectClass@ obj)
{
    for (int i = 0; i < obj.getNumPoints(); i++)
    {
        ProceduralPointClass@ point = obj.getPoint(i);
        point.position.y = global_extra_point_elevation + fmax(point.position.y, game.getGroundHeight(point.position) + global_min_point_elevation);
    }
}

void recalculatePointRotations(ProceduralObjectClass@ obj)
{
    // Loop all segments; for each segment, calculate the end-point orientation from the segment direction. Special case is point #0.
    //------------------------------------------------
    float MIN_HEIGHT_ABOVE_GROUND = 0.3f;
    for (int i = 1; i < obj.getNumPoints(); i++)
    {
        ProceduralPointClass@ point = obj.getPoint(i);
        ProceduralPointClass@ prev_point = obj.getPoint(i - 1);
        
        // Calc angle
        vector3 unitvec_road = (point.position - prev_point.position);
        unitvec_road.y = 0.f;
        unitvec_road.normalise();
        radian angle = radian(0.f);
        
        // -- the math part
        vector3 unitvec_x = vector3(1.f, 0.f, 0.f);
        angle = unitvec_x.angleBetween(unitvec_road);
        
        // -- the trial and error part - maybe there's something wrong with our procedural mesh generation
        if (unitvec_road.x < 0 && unitvec_road.z > 0)
        {
            angle = angle * radian(-1.f);
        }
        else if (unitvec_road.x > 0 && unitvec_road.z > 0)
        {
            angle = angle * radian(-1.f);
        }
        
        
        // Calc rotation
        quaternion rot_x = quaternion(0.f, vector3(1.f, 0.f, 0.f));
        quaternion rot_y = quaternion(angle, vector3(0.f, 1.f, 0.f));
        quaternion rot_z = quaternion(0.f, vector3(0.f, 0.f, 1.f));
        point.rotation = rot_x * rot_y * rot_z;
        
        // Special case - point #0
        if (i == 1)
        {
            prev_point.rotation = point.rotation;
        }
    }
}

void updateMeshRebuildInputEvents()
{
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    ProceduralObjectClass@ obj = roads.getObject(selected_road);    
    
    if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_REBUILD_MESH))
    {
        rebuildMesh(obj);
    }
}

//#endregion

//#region Manage roads tab
int selected_road = 0; // only for reading - use `setSelectedRoad()`

void setSelectedRoad(int road)
{
    if (road != selected_road)
    {
        selected_road = road;
        ProceduralObjectClass@ obj = game.getTerrain().getHandle().getProceduralManager().getHandle().getObject(selected_road);
        // AI export panel
        ai_export_first = min(ai_export_first, obj.getNumPoints() - 1);
        ai_export_last = min(ai_export_last, obj.getNumPoints() - 1);
        // Selected point
        int new_selected_point = clamp(0, selected_point, obj.getNumPoints());
        if (selected_point != new_selected_point)
        {
            setSelectedPoint(new_selected_point);
        }
    }
}

void drawRoadListPanel()
{
    // Basic info - num procedural objects and [+] button
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    ImGui::TextDisabled("There are " + roads.getNumObjects() + " road strips");
    ImGui::SameLine();
    if (ImGui::SmallButton("Add new road"))
    {
        // Add new road
        ProceduralObjectClass obj;
        obj.setName("road " + roads.getNumObjects() + " (from editor)");
        roads.addObject(obj);
        // Select the new road
        setSelectedRoad(roads.getNumObjects() - 1);
    }
    
    // Radio select of road to edit.
    ProceduralObjectClass@ obj_to_delete = null;
    for (int i = 0; i < roads.getNumObjects(); i++)
    {
        ProceduralObjectClass@ obj = roads.getObject(i);
        ImGui::PushID(i);
        if (ImGui::RadioButton(obj.getName(), i == selected_road))
        {
            setSelectedRoad(i);
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete"))
        {
            @obj_to_delete = @obj;
        }
        ImGui::PopID();
    }
    
    if (@obj_to_delete != null)
    {
        roads.removeObject(obj_to_delete);
        selected_road = min(selected_road, roads.getNumObjects() - 1);
    }
}

//#endregion

//#region AI Waypoints tab
int ai_import_first = 0;
int ai_import_last = 0;
int ai_import_available = 0;
bool ai_import_reverse = false;
int ai_export_first = 0;
int ai_export_last = 0;
bool ai_export_reverse = false;

void drawAiWaypointsExportPanel(ProceduralObjectClass@ obj)
{
    //Export road points as waypoints to survey map
    // -----------------------
    
    ImGui::PushID("ai export");
    
    
    if (obj.getNumPoints() > 0)
    {
        ImGui::Text("Current road has " + obj.getNumPoints() + " points");
        ImGui::SliderInt("First", ai_export_first, 0, obj.getNumPoints() - 1);
        ImGui::SliderInt("Last", ai_export_last, 0, obj.getNumPoints() - 1);
        if (ai_export_first > ai_export_last)
        {
            ImGui::Text("bad range!");
        }
        else
        {
            string btn_text = "Export " + ((ai_export_last - ai_export_first) + 1) + " waypoints";
            if (ImGui::Button(btn_text))
            {
                if (ai_export_reverse)
                {
                    for (int i = ai_export_last; i >= ai_export_first; i--)
                    {
                        game.addWaypoint(obj.getPoint(i).getHandle().position);
                    }
                }
                else
                {
                    for (int i = ai_export_first; i <= ai_export_last; i++)
                    {
                        game.addWaypoint(obj.getPoint(i).getHandle().position);
                    }
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Reverse", ai_export_reverse);
        }
    }
    else
    {
        ImGui::Text("The current road is empty - nothing to export!");
    }
    
    ImGui::PopID(); // "ai export"
}

void drawAiWaypointsImportPanel(ProceduralObjectClass@ obj)
{
    //Import road points from AI waypoints in survey map
    // ---------------------------------------------
    
    ImGui::PushID("ai import");
    
    
    array<vector3> waypoints = game.getWaypoints(0);
    
    // Resize range sliders if needed
    if (ai_import_available != int(waypoints.length()))
    {
        ai_import_first = min(ai_import_first, int(waypoints.length()) - 1);
        ai_import_last = min(ai_import_last, int(waypoints.length()) - 1);
    }
    
    // Draw export UI
    if (waypoints.length() > 0)
    {
        ImGui::Text("There are " + waypoints.length() + " waypoints available");
        
        ImGui::SliderInt("First", ai_import_first, 0, int(waypoints.length()) - 1);
        ImGui::SliderInt("Last", ai_import_last, 0, int(waypoints.length()) - 1);
        if (ai_import_first > ai_import_last)
        {
            ImGui::Text("bad range!");
        }
        else
        {
            string btn_text = "Import " + ((ai_import_last - ai_import_first) + 1) + " waypoints";
            if (ImGui::Button(btn_text))
            {
                if (ai_import_reverse)
                {
                    for (int i = ai_import_last; i >= ai_import_first; i--)
                    {
                        addPointToCurrentRoad(waypoints[i]);
                    }
                }
                else
                {
                    for (int i = ai_import_first; i <= ai_import_last; i++)
                    {
                        addPointToCurrentRoad(waypoints[i]);
                    }
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Reverse", ai_import_reverse);                
        }
    }
    else
    {
        ImGui::Text("No waypoints defined in survey map - nothing to import!");
    }        
    
    ImGui::PopID(); // "ai import"
}

void drawAiWaypointsPanel()
{
    ImGui::PushID("waypoints panel");
    ImGui::TextDisabled("This tool lets you convert between procedural road points and AI waypoints (editable using survey map).");
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    if (roads.getNumObjects() > 0)
    {
        ProceduralObjectClass@ obj = roads.getObject(selected_road);
        int barflags = 0;
        if (ImGui::BeginTabBar("AI tabs", barflags))
        {
            if (ImGui::BeginTabItem("Export road as AI waypoints"))
            {
                drawAiWaypointsExportPanel(obj);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Import AI waypoints as road"))
            {
                drawAiWaypointsImportPanel(obj);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        
    }
    else
    {
        ImGui::Text("There are no roads - nothing to import/export!");
    }
    
    ImGui::PopID(); // "waypoints panel"
}

// #endregion

//#region Road gizmos

float cfgGizmoPointRadius = 6.f;
int cfgGizmoPointNumSegments = 10;
color cfgGizmoPointHoveredColor = color(1.f, 0.6f, 0.3f, 0.75f);
color cfgGizmoPointSelectedColor = color(0.9f, 0.2f, 0.3f, 1.f);
color cfgGizmoPointIdleColor = color(0.9f, 0.1f, 0.1f, 1.f);

bool mouseDragRegistered = false;
float mouseDragThreshold = 1.f;

void updateGizmoMouseDrag()
{
    
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    ProceduralObjectClass@ obj = roads.getObject(selected_road);
    
    //ImGui::Text("DBG IsMouseDragging()=" + ImGui::IsMouseDragging());
    //ImGui::Text("DBG mouseDragRegisted: "+ mouseDragRegistered);
    
    if (mouseDragRegistered)
    {
        if (ImGui::IsMouseDragging(0, mouseDragThreshold))
        {
            vector3 mouse_tpos;
            
            if ( game.getMousePositionOnTerrain(mouse_tpos))
            {
                // Y is 'up'.
                ProceduralPointClass@ pp = obj.getPoint(selected_point);
                //vector3 old_pos = pp.position;
                vector3 new_pos = mouse_tpos + vector3(0,selected_point_elevation,0) ;
                //game.log("Updating road '"+obj.getName()+"', point '"+selected_point+"' position to X:"+new_pos.x+" Y:"+new_pos.y+" Z:"+new_pos.z+" (previously X:"+old_pos.x+" Y:"+old_pos.y+" Z:"+old_pos.z+")");
                pp.position = new_pos;
            }
        }
        // drag over?
        if (ImGui::IsMouseReleased(0))
        {
            mouseDragRegistered=false;
        }
    }
    
}



void visualizeRoad()
{
    TerrainClass@ terrain = game.getTerrain();
    ProceduralManagerClass@ roads = terrain.getProceduralManager();
    ProceduralObjectClass@ obj = roads.getObject(selected_road);
    ImDrawList@ drawlist = imgui_utils::ImGetDummyFullscreenWindow("Road editing gizmos");
    vector2 mouse_pos = game.getMouseScreenPosition();
    vector2 prev_screen_point(0,0);
    bool prev_point_visible = false;
    int closest_point = -1;
    int closest_point_distance = INT_MAX;
    
    updateGizmoMouseDrag();
    
    for (int i = 0; i < obj.getNumPoints(); i++)
    {
        // Draw this point
        ProceduralPointClass@ pp = obj.getPoint(i);
        vector3 point = pp.position;
        vector2 screen_point;
        bool point_visible = game.getScreenPosFromWorldPos(point, screen_point);
        
        if (point_visible)
        {        
            // Draw point
            
            
            if (i == selected_point)
            {
                drawlist.AddCircleFilled(screen_point, cfgGizmoPointRadius, cfgGizmoPointSelectedColor, cfgGizmoPointNumSegments );
            }
            else
            {
                drawlist.AddCircle(screen_point, cfgGizmoPointRadius, cfgGizmoPointIdleColor, cfgGizmoPointNumSegments , 2.f);
            }
            
            // hover is indicated by extra drawings
            if (i == hovered_point)
            {
                drawlist.AddCircle(screen_point, cfgGizmoPointRadius+4.f, cfgGizmoPointHoveredColor, cfgGizmoPointNumSegments , 4.f);
            }
            
            // Draw line from last point if visible
            if (prev_point_visible)
            {
                drawlist.AddLine(prev_screen_point, screen_point, color(0.8f, 0.8f, 0.2f, 1.f), 3);
            }            
            
            // Draw elevation marker (square)
            vector3 terrn_point = point;
            terrn_point.y = game.getGroundHeight(terrn_point);
            vector2 terrn_screen_point(0,0);
            bool terrn_point_visible = game.getScreenPosFromWorldPos(terrn_point, terrn_screen_point);
            if (terrn_point_visible)
            {
                
                if (i == selected_point)
                {
                    const float PAD = 4.f;
                    drawlist.AddRectFilled(
                    vector2(terrn_screen_point.x-PAD, terrn_screen_point.y-PAD),
                    vector2(terrn_screen_point.x+PAD, terrn_screen_point.y+PAD),
                    color(0.1f, 0.2f, 0.9f, 1.f), 0.f, 0);
                }
                else
                {
                    const float PAD = 3.f;
                    drawlist.AddRect(
                    vector2(terrn_screen_point.x-PAD, terrn_screen_point.y-PAD),
                    vector2(terrn_screen_point.x+PAD, terrn_screen_point.y+PAD),
                    color(0.1f, 0.2f, 0.9f, 1.f), 0.f, 0, 2.f);                
                }    
                
                // hover is indicated by extra elements
                if (i == hovered_point)
                {
                    const float PAD = 7.f;
                    drawlist.AddRect(
                    vector2(terrn_screen_point.x-PAD, terrn_screen_point.y-PAD),
                    vector2(terrn_screen_point.x+PAD, terrn_screen_point.y+PAD),
                    color(0.45f, 0.52f, 1.f, 0.75f), 0.f, 0, 5.f);   
                    if (ImGui::IsMouseClicked(0))
                    {
                        setSelectedPoint(hovered_point);
                        vector3 mousePosOnTerrain;
                        mouseDragRegistered = game.getMousePositionOnTerrain(/*[out]*/mousePosOnTerrain);
                    }                        
                }             
                
                // Draw elevation indicator line
                drawlist.AddLine(screen_point, terrn_screen_point, color(0.2f, 0.3f, 0.7f, 1.f), 3.f);
                
                // Accumulate mouse hover info
                int point_distance = getMouseShortestDistance(mouse_pos, terrn_screen_point);
                if (point_distance < closest_point_distance)
                {
                    closest_point = i;
                    closest_point_distance = point_distance;
                }                
            }
            
            if (i == selected_point)
            {
                selected_point_elevation = (point.y - terrn_point.y);
            }
            else if (i == hovered_point)
            {
                hovered_point_elevation = (point.y - terrn_point.y);
            }
        }
        
        // Move next
        prev_point_visible = point_visible;
        prev_screen_point = screen_point;
    }
    
    if (closest_point_distance < 15)
    {
        hovered_point = closest_point;
    }
    else
    {
        hovered_point = -1;
    }
}
//#endregion

//#region EditorCam
bool editorCam = true;
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

//#region Helper functions


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

int abs(int a)
{
    return (a < 0) ? -a : a;
}

int getMouseShortestDistance(vector2 mouse, vector2 target)
{
    int dx = abs(int(mouse.x) - int(target.x));
    int dy = abs(int(mouse.y) - int(target.y));
    return max(dx, dy);
}

//#endregion
