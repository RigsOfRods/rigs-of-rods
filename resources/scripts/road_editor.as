/*
    ---------------------------------------------------------------------------
                  Project Rigs of Rods (www.rigsofrods.org)
                            
                             PROCEDURAL ROAD EDITOR                                
    
    Procedural roads are static tracks generated from splines.
    Splines can be defined in .tobj files which come with terrain (.terrn2),
    but can also be created programmatically.
    
    INSTALLATION
        Put the script to Documents/My Games/Rigs of Rods/scripts
        and in game console, say `loadscript road_editor.as`.
    
    Procedural roads .tobj documentation:
    https://docs.rigsofrods.org/terrain-creation/old-terrn-subsystem/#procedural-roads
    
    Scripting documentation:
    https://developer.rigsofrods.org/d4/d07/group___script2_game.html
     
    ---------------------------------------------------------------------------
*/


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
        this.updateRoad();
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
            this.drawRoadList();     
        }        
        
        // Slider for selecting nodes
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        if (roads.getNumObjects() > 0)
        {
            ImGui::PushID("road edit box");
            
            ProceduralObjectClass@ obj = roads.getObject(m_selected_road);
            ImGui::Text("Road: " + obj.getName());
            ImGui::Text("Total points: " + obj.getNumPoints());
            if (obj.getNumPoints() > 0)
            {
                ImGui::TextDisabled("Use slider or left-click road point to select it.");
                ImGui::TextDisabled("Press 'M' to move the selected point to mouse position");
                int slider_val = m_selected_point;
                if (ImGui::SliderInt("", slider_val, 0, obj.getNumPoints() - 1))
                {
                    this.setSelectedPoint(slider_val);
                }
                if (ImGui::Button("Go to point"))
                {
                    this.goToPoint(obj, m_selected_point);
                }
                ImGui::SameLine();            
                ImGui::Checkbox("Auto", m_selected_point_auto_goto);
            }
            
            ImGui::TextDisabled("Add new road point at the character position and link it to selected road point (if any).");
            if (ImGui::Button("Insert new point"))
            {
                ProceduralPointClass@ point = ProceduralPointClass();
                point.position = game.getPersonPosition();                
                
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
            
            if (obj.getNumPoints() > 0)
            {
                if (ImGui::Button("Remove selected point"))
                {
                    obj.deletePoint(m_selected_point);
                }
            
                if (ImGui::Button("> Rebuild road mesh <"))
                {
                    roads.removeObject(obj); // Clears the existing mesh
                    this.recalculateRotations(obj);
                    roads.addObject(obj); // Generates new mesh
                } 
            }                

            ImGui::PopID(); // road edit box
        }
        
        // End window
        ImGui::End();
    }
    
    void recalculateRotations(ProceduralObjectClass@ obj)
    {
        // Loop all segments; for each segment, calculate the end-point orientation from the segment direction. Special case is point #0.
        //------------------------------------------------
        float MIN_HEIGHT_ABOVE_GROUND = 0.3f;
        for (int i = 1; i < obj.getNumPoints(); i++)
        {
            ProceduralPointClass@ point = obj.getPoint(i);
            point.position.y = fmax(point.position.y, game.getGroundHeight(point.position) + MIN_HEIGHT_ABOVE_GROUND);
            ProceduralPointClass@ prev_point = obj.getPoint(i - 1);
            prev_point.position.y = fmax(prev_point.position.y, game.getGroundHeight(prev_point.position) + MIN_HEIGHT_ABOVE_GROUND);
            
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
    
    void drawRoadList()
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
            obj.setName("from editor " + roads.getNumObjects());
            roads.addObject(obj);
            // Select the new road
            m_selected_road = roads.getNumObjects() - 1;
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
                if (m_selected_road != i)
                {
                    m_selected_point = clamp(0, m_selected_point, obj.getNumPoints());
                }
                m_selected_road = i;
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

    void visualizeRoad()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        ProceduralObjectClass@ obj = roads.getObject(m_selected_road);
        ImDrawList@ drawlist = getDummyFullscreenWindow("Road editing gizmos");
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
    
    void updateRoad()
    {
        TerrainClass@ terrain = game.getTerrain();
        ProceduralManagerClass@ roads = terrain.getProceduralManager();
        ProceduralObjectClass@ obj = roads.getObject(m_selected_road);    
    
        // Reused Hotkey 'M'
        vector3 mouse_tpos;
        if (inputs.getEventBoolValue(EV_COMMON_TOGGLE_TRUCK_BEACONS)
            && game.getMousePositionOnTerrain(mouse_tpos))
        {
            // Y is 'up'.
            ProceduralPointClass@ pp = obj.getPoint(m_selected_point);
            //vector3 old_pos = pp.position;
            vector3 new_pos = mouse_tpos + vector3(0,m_selected_point_elevation,0);
            //game.log("Updating road '"+obj.getName()+"', point '"+m_selected_point+"' position to X:"+new_pos.x+" Y:"+new_pos.y+" Z:"+new_pos.z+" (previously X:"+old_pos.x+" Y:"+old_pos.y+" Z:"+old_pos.z+")");
            pp.position = new_pos;
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
 
    int m_selected_road = 0;
    int m_selected_point = 0;
    int m_hovered_point = 0;
    float m_selected_point_elevation = 0.f;
    float m_hovered_point_elevation = 0.f;  
    bool m_selected_point_auto_goto = false;
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

ImDrawList@ getDummyFullscreenWindow(const string&in name)
{
    // Dummy fullscreen window to draw to
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowPos(vector2(0,0));
    ImGui::SetNextWindowSize(game.getDisplaySize());
    ImGui::PushStyleColor(/*ImGuiCol_WindowBg*/2, color(0.f,0.f,0.f,0.f)); // Fully transparent background!
    ImGui::Begin(name, /*open:*/true, window_flags);
    ImDrawList@ drawlist = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg

    return drawlist;    
}

int getMouseShortestDistance(vector2 mouse, vector2 target)
{
    int dx = abs(int(mouse.x) - int(target.x));
    int dy = abs(int(mouse.y) - int(target.y));
    return max(dx, dy);
}
