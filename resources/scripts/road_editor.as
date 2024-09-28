/*
    ---------------------------------------------------------------------------
                  Project Rigs of Rods (www.rigsofrods.org)
                            
                             PROCEDURAL ROAD EDITOR                                
    
    Procedural roads are static tracks generated from splines.
    Splines can be defined in .tobj files which come with terrain (.terrn2),
    but can also be created programmatically.
    
    To run, open in-game console (Hotkey ~) and say 'loadscript road_editor.as'
    
    Procedural roads .tobj documentation:
    https://docs.rigsofrods.org/terrain-creation/old-terrn-subsystem/#procedural-roads
    
    Scripting documentation:
    https://developer.rigsofrods.org/d4/d07/group___script2_game.html
     
    ---------------------------------------------------------------------------
*/

#include "imgui_utils.as"

/*
    ---------------------------------------------------------------------------
    Global variables
*/

CVarClass@  g_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
CVarClass@  g_sim_state = console.cVarFind("sim_state"); // 0=off, 1=running, 2=paused, 3=terrain editor, see SimState in Application.h
CVarClass@  g_mp_state = console.cVarFind("mp_state"); // 0=disabled, 1=connecting, 2=connected, see MpState in Application.h
TerrainEditor g_terrain_editor;

/*
    ---------------------------------------------------------------------------
    Script setup function - invoked once when script is loaded.
*/
void main()
{
    log("Road editor loaded! Enter terrain editing mode (hotkey: " + inputs.getEventCommandTrimmed(EV_COMMON_TOGGLE_TERRAIN_EDITOR) + ") to use it.");
}

/*
    ---------------------------------------------------------------------------
    Script update function - invoked once every rendered frame,
    with elapsed time (delta time, in seconds) as parameter.
*/
void frameStep(float dt)
{
    if (g_app_state.getInt() == 2 // simulation
        && g_sim_state.getInt() == 3 // terrain editor mode
        && g_mp_state.getInt() == 0) // singleplayer
    {
        g_terrain_editor.frameStep(dt);
    }
}

/*
    ---------------------------------------------------------------------------
    Custom objects
*/

class TerrainEditor
{
    void frameStep(float dt)
    {
        this.drawWindow();
        
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        if (roads.getNumObjects() == 0)
        {
            return; // nothing more to do
        }
        else
        {
            ProceduralObjectClass@ obj = roads.getObject(m_selected_road);
            if (obj.getNumPoints() == 0)
            {
                return; // nothing more to do
            }
        }
        this.visualizeRoad();
        this.updateInputEvents();
    }

    void drawWindow()
    {
        // Open demo window
        ImGui::SetNextWindowPos(vector2(25, 120)); // Make space for FPS box.
        int flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse; 
        if (!ImGui::Begin("Road editor", /*open:*/true, flags))
        {
            return;
        }
        ImGui::Dummy(vector2(200.f, 1.f)); // force width
        
        if (ImGui::CollapsingHeader("Manage roads"))
        {
            this.drawRoadListPanel();     
        }

        if (ImGui::CollapsingHeader("AI Waypoints"))
        {
            this.drawAiWaypointsPanel();
        }
        
        this.drawRoadEditPanel();
        
        // End window
        ImGui::End();
    }
    
    void drawRoadEditPanel()
    {
        ImGui::PushID("road edit box");
    
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        if (roads.getNumObjects() > 0)
        {
            ProceduralObjectClass@ obj = roads.getObject(m_selected_road);
            ImGui::Text("Road: " + obj.getName());
            ImGui::Text("Total points: " + obj.getNumPoints());
            if (obj.getNumPoints() > 0)
            {
                ImGui::TextDisabled("Use slider or left-click road point to select it.");
                ImGui::TextDisabled("Press hotkey '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_SET_POS) + "' to move the selected point to mouse position");
                int slider_val = m_selected_point;
                if (ImGui::SliderInt("", slider_val, 0, obj.getNumPoints() - 1))
                {
                    this.setSelectedPoint(slider_val);
                }
                if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_GOTO)
                    || ImGui::Button("Go to point (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_GOTO) + "')"))
                {
                    this.goToPoint(obj, m_selected_point);
                }
                ImGui::SameLine();            
                ImGui::Checkbox("Auto", m_selected_point_auto_goto);
            }
            
            ImGui::TextDisabled("Add new road point at the character position and link it to selected road point (if any).");
            if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_INSERT)
                || ImGui::Button("Insert new point (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_INSERT) + "')"))
            {
                this.addPointToCurrentRoad(game.getPersonPosition());
            }
            
            if (obj.getNumPoints() > 0)
            {
                if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_POINT_DELETE)
                    || ImGui::Button("Remove selected point (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_POINT_DELETE) + "')"))
                {
                    this.deletePointFromCurrentRoad(m_selected_point);
                }
            }
            
            if (ImGui::CollapsingHeader("Point properties"))
            {
                this.drawPointPropertiesPanel(obj);
            }              
            
            if (ImGui::CollapsingHeader("Mesh rebuild"))
            {
                this.drawMeshRebuildPanel(obj);
            }         
        }
        ImGui::PopID(); // "road edit box"
    }
    
    void drawPointPropertiesPanel(ProceduralObjectClass@ obj)
    {
        ImGui::PushID("point properties");
        if (obj.getNumPoints() > 0)
        {
            ProceduralPointClass@ point = obj.getPoint(m_selected_point);
        
            ImGui::SetNextItemWidth(130.f);
            ImGui::InputFloat("Elevation (meters)", point.position.y);
            
            ImGui::SetNextItemWidth(100.f);
            ImGui::InputFloat("Width (meters)", point.width);   

            ImGui::SetNextItemWidth(100.f);
            ImGui::InputFloat("Border width (meters)", point.border_width);  
            
            ImGui::SetNextItemWidth(100.f);
            ImGui::InputFloat("Border height (meters)", point.border_height);

            ImGui::Text("Type:");
            // Types supported by TOBJ format
            if (ImGui::RadioButton("(automatic)", point.type == ROAD_AUTOMATIC))
            {
                point.type = ROAD_AUTOMATIC;
                point.pillar_type = 0;
            }
            if (ImGui::RadioButton("flat (no border)", point.type == ROAD_FLAT))
            {
                point.type = ROAD_FLAT;
                point.pillar_type = 0;
            }  
            if (ImGui::RadioButton("left (border on left)", point.type == ROAD_LEFT))
            {
                point.type = ROAD_LEFT;
                point.pillar_type = 0;
            }   
            if (ImGui::RadioButton("right (border on right)", point.type == ROAD_RIGHT))
            {
                point.type = ROAD_RIGHT;
                point.pillar_type = 0;
            }      
            if (ImGui::RadioButton("both (with borders)", point.type == ROAD_BOTH))
            {
                point.type = ROAD_BOTH;
                point.pillar_type = 0;
            }    
            if (ImGui::RadioButton("bridge (with pillars)", point.type == ROAD_BRIDGE && point.pillar_type == 1))
            {
                point.type = ROAD_BRIDGE;
                point.pillar_type = 1;
            }
            if (ImGui::RadioButton("bridge_no_pillars", point.type == ROAD_BRIDGE && point.pillar_type == 0))
            {
                point.type = ROAD_BRIDGE;
                point.pillar_type = 0;
            }
            if (ImGui::RadioButton("monorail (with pillars)", point.type == ROAD_BRIDGE && point.pillar_type == 2))
            {
                point.type = ROAD_MONORAIL;
                point.pillar_type = 2;
            }
            if (ImGui::RadioButton("monorail2 (no pillars)", point.type == ROAD_BRIDGE && point.pillar_type == 0))
            {
                point.type = ROAD_MONORAIL;
                point.pillar_type = 0;
            } 
            // End of types supported by TOBJ format
        }
        else
        {
            ImGui::Text("Road has no points - nothing to edit!");
        }        
        
        ImGui::PopID(); //"point properties"
    }
    
    void drawMeshRebuildPanel(ProceduralObjectClass@ obj)
    {
        ImGui::PushID("mesh rebuild");
        
        ProceduralManagerClass@ roads = game.getTerrain().getHandle().getProceduralManager();
    
        if (obj.getNumPoints() > 0)
        {
            ImGui::SetNextItemWidth(100.f);
            ImGui::InputFloat("Common minimum point elevation (meters)", m_global_min_point_elevation);
        
            ImGui::SetNextItemWidth(100.f);
            ImGui::InputFloat("Common extra point elevation (meters)", m_global_extra_point_elevation);            
        
            ImGui::SetNextItemWidth(120.f);
            ImGui::InputInt("Num smoothing splits (0=off)", obj.smoothing_num_splits);
        
            // NOTE: hotkey is processed in `updateInputEvents()` to be independent of the UI
            if (ImGui::Button("Rebuild road mesh (hotkey: '" + inputs.getEventCommandTrimmed(EV_ROAD_EDITOR_REBUILD_MESH) + "')"))
            {
                this.rebuildMesh(obj);
            } 
        }
        else
        {
            ImGui::Text("Road has no points - nothing to generate!");
        }
        
        ImGui::PopID(); //"mesh rebuild"
    }
    
    void recalculatePointElevations(ProceduralObjectClass@ obj)
    {
        for (int i = 0; i < obj.getNumPoints(); i++)
        {
            ProceduralPointClass@ point = obj.getPoint(i);
            point.position.y = m_global_extra_point_elevation + fmax(point.position.y, game.getGroundHeight(point.position) + m_global_min_point_elevation);
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
    
    void drawRoadListPanel()
    {
        // Basic info - num procedural objects and [+] button
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        ImGui::Text("There are " + roads.getNumObjects() + " road strips");
        ImGui::SameLine();
        if (ImGui::Button("Add new"))
        {
            // Add new road
            ProceduralObjectClass obj;
            obj.setName("road " + roads.getNumObjects() + " (from editor)");
            roads.addObject(obj);
            // Select the new road
            this.setSelectedRoad(roads.getNumObjects() - 1);
        }
        ImGui::Separator();
        
        // Radio select of road to edit.
        ProceduralObjectClass@ obj_to_delete = null;
        for (int i = 0; i < roads.getNumObjects(); i++)
        {
            ProceduralObjectClass@ obj = roads.getObject(i);
            ImGui::PushID(i);
            if (ImGui::RadioButton(obj.getName(), i == m_selected_road))
            {
                this.setSelectedRoad(i);
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
            m_selected_road = min(m_selected_road, roads.getNumObjects() - 1);
        }
        ImGui::Separator();
    }
    
    void drawAiWaypointsExportPanel(ProceduralObjectClass@ obj)
    {
        ImGui::PushID("ai export");
        ImGui::Text("Export road points as waypoints to survey map");
        
        if (obj.getNumPoints() > 0)
        {
            ImGui::Text("Current road has " + obj.getNumPoints() + " points");
            ImGui::SliderInt("First", m_ai_export_first, 0, obj.getNumPoints() - 1);
            ImGui::SliderInt("Last", m_ai_export_last, 0, obj.getNumPoints() - 1);
            if (m_ai_export_first > m_ai_export_last)
            {
                ImGui::Text("bad range!");
            }
            else
            {
                string btn_text = "Export " + ((m_ai_export_last - m_ai_export_first) + 1) + " waypoints";
                if (ImGui::Button(btn_text))
                {
                    if (m_ai_export_reverse)
                    {
                        for (int i = m_ai_export_last; i >= m_ai_export_first; i--)
                        {
                            game.addWaypoint(obj.getPoint(i).getHandle().position);
                        }
                    }
                    else
                    {
                        for (int i = m_ai_export_first; i <= m_ai_export_last; i++)
                        {
                            game.addWaypoint(obj.getPoint(i).getHandle().position);
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Reverse", m_ai_export_reverse);
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
        ImGui::PushID("ai import");
        
        ImGui::Text("Import road points from AI waypoints in survey map");
        array<vector3> waypoints = game.getWaypoints(0);
        
        // Resize range sliders if needed
        if (m_ai_import_available != int(waypoints.length()))
        {
            m_ai_import_first = min(m_ai_import_first, int(waypoints.length()) - 1);
            m_ai_import_last = min(m_ai_import_last, int(waypoints.length()) - 1);
        }
        
        // Draw export UI
        if (waypoints.length() > 0)
        {
            ImGui::Text("There are " + waypoints.length() + " waypoints available");

            ImGui::SliderInt("First", m_ai_import_first, 0, int(waypoints.length()) - 1);
            ImGui::SliderInt("Last", m_ai_import_last, 0, int(waypoints.length()) - 1);
            if (m_ai_import_first > m_ai_import_last)
            {
                ImGui::Text("bad range!");
            }
            else
            {
                string btn_text = "Import " + ((m_ai_import_last - m_ai_import_first) + 1) + " waypoints";
                if (ImGui::Button(btn_text))
                {
                    if (m_ai_import_reverse)
                    {
                        for (int i = m_ai_import_last; i >= m_ai_import_first; i--)
                        {
                            this.addPointToCurrentRoad(waypoints[i]);
                        }
                    }
                    else
                    {
                        for (int i = m_ai_import_first; i <= m_ai_import_last; i++)
                        {
                            this.addPointToCurrentRoad(waypoints[i]);
                        }
                    }
                }
                ImGui::SameLine();
                ImGui::Checkbox("Reverse", m_ai_import_reverse);                
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
            ProceduralObjectClass@ obj = roads.getObject(m_selected_road);
            ImGui::Separator();
            this.drawAiWaypointsExportPanel(obj);
            ImGui::Separator();
            this.drawAiWaypointsImportPanel(obj);
        }
        else
        {
            ImGui::Text("There are no roads - nothing to import/export!");
        }
        
        ImGui::PopID(); // "waypoints panel"
    }

    void visualizeRoad()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        ProceduralObjectClass@ obj = roads.getObject(m_selected_road);
        ImDrawList@ drawlist = imgui_utils::ImGetDummyFullscreenWindow("Road editing gizmos");
        vector2 mouse_pos = game.getMouseScreenPosition();
        vector2 prev_screen_point(0,0);
        bool prev_point_visible = false;
        int closest_point = -1;
        int closest_point_distance = INT_MAX;
        
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
                if (i == m_selected_point)
                {
                    drawlist.AddCircleFilled(screen_point, 7.f, color(0.9f, 0.5f, 0.2f, 1.f), 12);
                }
                else if (i == m_hovered_point)
                {
                    drawlist.AddCircle(screen_point, 9.f, color(0.9f, 0.2f, 0.3f, 1.f), 12, 4.f);
                }
                else
                {
                    drawlist.AddCircle(screen_point, 5.f, color(0.9f, 0.1f, 0.1f, 1.f), 12, 2.f);
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
                    
                    if (i == m_selected_point)
                    {
                        const float PAD = 4.f;
                        drawlist.AddRectFilled(
                            vector2(terrn_screen_point.x-PAD, terrn_screen_point.y-PAD),
                            vector2(terrn_screen_point.x+PAD, terrn_screen_point.y+PAD),
                            color(0.1f, 0.2f, 0.9f, 1.f), 0.f, 0);
                    }
                    else if (i == m_hovered_point)
                    {
                        const float PAD = 6.f;
                        drawlist.AddRect(
                            vector2(terrn_screen_point.x-PAD, terrn_screen_point.y-PAD),
                            vector2(terrn_screen_point.x+PAD, terrn_screen_point.y+PAD),
                            color(0.2f, 0.3f, 0.9f, 1.f), 0.f, 0, 5.f);   
                        if (ImGui::IsMouseClicked(0))
                        {
                            this.setSelectedPoint(m_hovered_point);
                        }                        
                    }
                    else
                    {
                        const float PAD = 3.f;
                        drawlist.AddRect(
                            vector2(terrn_screen_point.x-PAD, terrn_screen_point.y-PAD),
                            vector2(terrn_screen_point.x+PAD, terrn_screen_point.y+PAD),
                            color(0.1f, 0.2f, 0.9f, 1.f), 0.f, 0, 2.f);                
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
                
                if (i == m_selected_point)
                {
                    m_selected_point_elevation = (point.y - terrn_point.y);
                }
                else if (i == m_hovered_point)
                {
                    m_hovered_point_elevation = (point.y - terrn_point.y);
                }
            }
            
            // Move next
            prev_point_visible = point_visible;
            prev_screen_point = screen_point;
        }
        
        if (closest_point_distance < 15)
        {
            m_hovered_point = closest_point;
        }
        else
        {
            m_hovered_point = -1;
        }
    }
    
    void updateInputEvents()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        ProceduralObjectClass@ obj = roads.getObject(m_selected_road);    
    
        vector3 mouse_tpos;
        if (inputs.getEventBoolValue(EV_ROAD_EDITOR_POINT_SET_POS)
            && game.getMousePositionOnTerrain(mouse_tpos))
        {
            // Y is 'up'.
            ProceduralPointClass@ pp = obj.getPoint(m_selected_point);
            //vector3 old_pos = pp.position;
            vector3 new_pos = mouse_tpos + vector3(0,m_selected_point_elevation,0);
            //game.log("Updating road '"+obj.getName()+"', point '"+m_selected_point+"' position to X:"+new_pos.x+" Y:"+new_pos.y+" Z:"+new_pos.z+" (previously X:"+old_pos.x+" Y:"+old_pos.y+" Z:"+old_pos.z+")");
            pp.position = new_pos;
        }
        
        if (inputs.getEventBoolValueBounce(EV_ROAD_EDITOR_REBUILD_MESH))
        {
            this.rebuildMesh(obj);
        }
    }
    
    void rebuildMesh(ProceduralObjectClass@ obj)
    {
        ProceduralManagerClass@ roads = game.getTerrain().getHandle().getProceduralManager();
    
        roads.removeObject(obj); // Clears the existing mesh
        this.recalculatePointElevations(obj);
        this.recalculatePointRotations(obj);
        roads.addObject(obj); // Generates new mesh
        // Because we removed and re-added the ProceduralObject from/to the manager,
        // it got new index, so our selection is invalid. Update it.
        this.setSelectedRoad(roads.getNumObjects() - 1);
    }
    
    void addPointToCurrentRoad(vector3 pos)
    {
        ProceduralObjectClass@ obj = game.getTerrain().getHandle().getProceduralManager().getHandle().getObject(m_selected_road);
    
        ProceduralPointClass@ point = ProceduralPointClass();
        if (obj.getNumPoints() > 0)
        {
            ProceduralPointClass@ template = obj.getPoint(m_selected_point);
            point.type = template.type;
            point.pillar_type = template.pillar_type;
            point.width = template.width;
            point.border_width = template.border_width;
            point.border_height = template.border_height;
        }
        point.position = pos;            
        
        if (obj.getNumPoints() == 0 || m_selected_point == obj.getNumPoints() - 1)
        {
            obj.addPoint(point);
            m_selected_point = obj.getNumPoints() - 1;
        }
        else
        {
            obj.insertPoint(m_selected_point, point);
        }
    }  

    void deletePointFromCurrentRoad(int pos)
    {
        ProceduralObjectClass@ obj = game.getTerrain().getHandle().getProceduralManager().getHandle().getObject(m_selected_road);
        
        obj.deletePoint(pos);
        if (pos >= obj.getNumPoints())
        {
            if (obj.getNumPoints() > 0)
                this.setSelectedPoint(obj.getNumPoints() - 1);
            else
                m_selected_point = 0;
        }
    }
    
    void goToPoint(ProceduralObjectClass@ obj, int point_idx)
    {
        ProceduralPointClass@ point = obj.getPoint(point_idx);
        game.setPersonPosition(point.position);
        game.setPersonRotation(point.rotation.getYaw()); // I have no idea if this is correct.
    }
    
    void setSelectedPoint(int point)
    {
        if (point != m_selected_point)
        {
            m_selected_point = point;
            if (m_selected_point_auto_goto)
            {
                TerrainClass@ terrain = game.getTerrain();
                ProceduralManagerClass@ proc_mgr = terrain.getProceduralManager();
                ProceduralObjectClass@ proc_obj = proc_mgr.getObject(m_selected_road);
                
                this.goToPoint(proc_obj, m_selected_point);
            }
        }
    }
    
    void setSelectedRoad(int road)
    {
        if (road != m_selected_road)
        {
            m_selected_road = road;
            ProceduralObjectClass@ obj = game.getTerrain().getHandle().getProceduralManager().getHandle().getObject(m_selected_road);
            // AI export panel
            m_ai_export_first = min(m_ai_export_first, obj.getNumPoints() - 1);
            m_ai_export_last = min(m_ai_export_last, obj.getNumPoints() - 1);
            // Selected point
            int new_selected_point = clamp(0, m_selected_point, obj.getNumPoints());
            if (m_selected_point != new_selected_point)
            {
                this.setSelectedPoint(new_selected_point);
            }
        }
    }
 
    int m_selected_road = 0;
    int m_selected_point = 0;
    int m_hovered_point = 0;
    float m_selected_point_elevation = 0.f;
    float m_hovered_point_elevation = 0.f;  
    bool m_selected_point_auto_goto = false;
    
    // Waypoints panel
    int m_ai_import_first = 0;
    int m_ai_import_last = 0;
    int m_ai_import_available = 0;
    bool m_ai_import_reverse = false;
    int m_ai_export_first = 0;
    int m_ai_export_last = 0;
    bool m_ai_export_reverse = false;
    
    // Mesh generation panel
    float m_global_extra_point_elevation = 0; // Added to all points
    float m_global_min_point_elevation = 0.3; // Added to lower points
}

/*
    ---------------------------------------------------------------------------
    Helper functions
*/

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
