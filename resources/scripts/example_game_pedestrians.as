/// \title Example pedestrian NPCs (characters following path network)
/// based on OGRE example - character posing (skeletal animations)

#include "math_utils.as"

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

string gErrorStr = "";

//#region Path network data
// Waypoints are global, paths are defined by connecting 2 waypoints
// this is much like verts/indices in a mesh, except there's a edge-list instead of a triangle-list.
class NpcWaypoint
{
    vector3 position;
    bool deleted; // Waypoints are reffered to by array positions, so deleted ones must stay until pruned.
    bool selected;
NpcWaypoint(){deleted=false; selected=false;}
NpcWaypoint(vector3 pos){deleted=false; selected=false; position=pos;}
};
array<NpcWaypoint> npcWaypoints;
class NpcPath
{
    int wp1, wp2;
NpcPath(){wp1=-1; wp2=-1;}
NpcPath(int wayp1, int wayp2){wp1=wayp1; wp2 = wayp2;}
};
array<NpcPath> npcPaths;
//#endregion

// #region Path network manip
bool pathExistsBetween(int wp1, int wp2)
{
    if (wp1 < 0 || wp1 > int(npcWaypoints.length()) || wp2 < 0 || wp2 > int(npcWaypoints.length()))
    {
        gErrorStr = "pathExistsBetween() - bad IDs! wp1="+wp1+", wp2="+wp2+", len:"+int(npcWaypoints.length());
        return false;
    }
    
    for (int i = 0; i<int(npcPaths.length()); i++)
    {
        if ( (npcPaths[i].wp1 == wp1 && npcPaths[i].wp2 == wp2)
        || (npcPaths[i].wp1 == wp2 && npcPaths[i].wp2 == wp1 ))
        {
            return true; // already exists.
        }
    }
    return false;
}
void addPathBetweenWaypoints(int wp1, int wp2)
{
if (pathExistsBetween(wp1,wp2)) { return; }
    // not existing
    npcPaths.insertLast(NpcPath(wp1,wp2));
}
void addPathBetweenSelected()
{
    if (lastNumSelected < 2) 
    {
        game.log("addPathBetweenSelected() - at least 2 waypoints must be selected!");
    }
    
    // each-to-each (i=front, j=rear)
    int numPathsBefore =int(npcPaths.length());
    for (int i=0; i<int(npcWaypoints.length()); i++)
    {
    if (!npcWaypoints[i].selected) {continue;}
        for (int j=i; j<int(npcWaypoints.length()); j++)
        {
        if (i==j || !npcWaypoints[j].selected) {continue;}
            addPathBetweenWaypoints(i,j);
        }
        
    }
    game.log("addPathBetweenSelected() - added " + (int(npcPaths.length()) - numPathsBefore) + " new paths");
}
//#endregion

//#region Path editor UI
#include "gridviewer_utils.as"
gridviewer_utils::GridViewer gVertsViewer;

// waypoint display conf
float gBottomBarHeight = 0;
// surveymap texture
Ogre::TexturePtr gTex;
color gColorViewerImageTint(1,1,1,1); // white

void setupPathEditor()
{
    gVertsViewer.childWindowID="Waypoints and paths";
    // axes are X=0, Y=1, Z=2, Y is up, we want top-down view (XZ)
    gVertsViewer.hAxis = 0; // X
    gVertsViewer.vAxis = 2; // Z
    
    // load surveymap texture
    gTex = Ogre::TextureManager::getSingleton().load("SurveyMapStatic", "OgreAutodetect");
    if (gTex.isNull())
    {
        gErrorStr = "couldn't load tex";
        return;
    }    
}

void drawPathEditor()
{
    vector2 windowSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();    
    float spaceUnderViewer = 0;
    vector2 viewerSize = vector2(windowSize.x , windowSize.y - (gBottomBarHeight+spaceUnderViewer));
    
    gVertsViewer.begin(viewerSize);
    
    // draw the texture - size MUST match terrain or waypoint positions will be off.
    // FIXME: actually get terrain size from the game!
    vector3 terrainSize(1024,1,1024); // dummy for testing on simple2
    vector2 imgImgMin = gVertsViewer.localToScreenPos(vector3(0,0,0));
    vector2 imgImgMax = gVertsViewer.localToScreenPos(terrainSize);
    ImGui::GetWindowDrawList().AddImage(gTex, imgImgMin, imgImgMax, vector2(0,0), vector2(1,1), gColorViewerImageTint);
    
    // draw paths
    for (int i = 0; i<int(npcPaths.length()); i++)
    {
        vector2 pos1=gVertsViewer.localToScreenPos(npcWaypoints[npcPaths[i].wp1].position);
        vector2 pos2=gVertsViewer.localToScreenPos(npcWaypoints[npcPaths[i].wp2].position);
        ImGui::GetWindowDrawList().AddLine(pos1,pos2,cfgPathColor);
        
    }
    
    drawWaypointHighlights();
    
    updateWaypointMouseControls();
    
    vector2 controlsCursor = gVertsViewer.endGridBeginControls();
    
    controlsCursor = drawWaypointSelectionUI(controlsCursor);
    
    // Walker management
    ImGui::SetCursorPos(controlsCursor);
    if (ImGui::SmallButton("release a walker!"))
    {
        releaseWalker();
    }
    
    gVertsViewer.end();
    
}

//#endregion

 
// #region CONFIG

color cfgNodeColor = color(0.9, 0.8, 0.5, 1);
color cfgPathColor = color(0.8, 0.85, 0.5, 1);
float cfgNodeRadius = 3.f;
int cfgNodeNumSegments = 5;

int cfgNodeHoverMaxCursorDist = 10; // Only confirm the hovered state if the cursor is less than 10 pixels away from it in any direction.
color cfgNodeHoverColor = color(1,0,0,1);
float cfgNodeHoverRadius = 5.f;
float cfgNodeHoverThickness = 2.f;
color cfgSelectedNodeColor = color(0.8f,1.f,0.1f,1.f);
float cfgSelectedNodeSize = 7.f; // drawn as rectangle

void drawConfigUI()
{
    
    ImGui::InputFloat("Node radius", cfgNodeRadius);
    ImGui::InputInt("Node num segments", cfgNodeNumSegments);
    //DOC: bool ColorEdit3(const string&in, color&inout)
    ImGui::ColorEdit3("Node color", cfgNodeColor);
    ImGui::ColorEdit3("Path color", cfgPathColor);
    
    
    ImGui::ColorEdit3("Hover color", cfgNodeHoverColor);
    ImGui::InputFloat("Hover radius", cfgNodeHoverRadius);
    ImGui::InputFloat("Hover thickness", cfgNodeHoverThickness);
    ImGui::InputInt("Hover max cursor dist.", cfgNodeHoverMaxCursorDist);
    
    ImGui::ColorEdit3("Selected node color", cfgSelectedNodeColor);
    ImGui::InputFloat("Selected node size", cfgSelectedNodeSize);
}
//#endregion

// #region Waypoint interaction

int mouseHoveredNodeID = -1;
string mouseHoverDebugString;
int lastNumSelected = 0;

void updateWaypointMouseControls()
{
    // Left mouse button click = flip the selection state of the waypoint
    if (mouseHoveredNodeID != -1 && ImGui::IsMouseClicked(0))
    {
        npcWaypoints[mouseHoveredNodeID].selected = !npcWaypoints[mouseHoveredNodeID].selected;
    }
    
    // Right mouse button click = add new waypoint
    if (ImGui::IsMouseClicked(1))
    {
        
        vector3 desiredPos = gVertsViewer.screenToLocalPos(ImGui::GetMousePos());
        game.log("right mouse clicked! ");
        npcWaypoints.insertLast(NpcWaypoint(desiredPos));
    }
}

vector2 drawWaypointSelectionUI(vector2 controlsCursor)
{
    // First line
    ImGui::SetCursorPos(controlsCursor);
    
    ImGui::TextColored( cfgNodeColor, "Total waypoints: "+npcWaypoints.length() + " (use right mouse button to create)");
    
    // Next line
    controlsCursor.y += ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetCursorPos(controlsCursor);
    
    int totalSelected = 0;
    string strSelected = "";
    string strJoiner = "";
    for (uint i=0; i < npcWaypoints.length(); i++)
    {
        if (npcWaypoints[i].selected)
        {
            strSelected += (strJoiner + i);
            strJoiner = ",";
            totalSelected++;
        }
    }
    ImGui::TextDisabled("Selected waypoints (" + totalSelected + ")"); // + mouseHoverDebugString);
    lastNumSelected = totalSelected; // for editing checks
    //   ImGui::Text(strSelected);
    ImGui::SameLine();
    if (ImGui::SmallButton("Select all"))
    {
        for (uint i=0; i < npcWaypoints.length(); i++)
        {
            npcWaypoints[i].selected = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("UnSelect all"))
    {
        for (uint i=0; i < npcWaypoints.length(); i++)
        {
            npcWaypoints[i].selected = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Flip selection"))
    {
        for (uint i=0; i < npcWaypoints.length(); i++)
        {
            npcWaypoints[i].selected = !npcWaypoints[i].selected;
        }
    }
    
    // Next line
    controlsCursor.y += ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetCursorPos(controlsCursor);
    
    // Path editing buttons
    ImGui::TextColored(cfgPathColor, "Path tools (total paths: "+int(npcPaths.length())+")");
    ImGui::SameLine();
    if (ImGui::Button("Add between selected"))
    {
        addPathBetweenSelected();
    }    
    
    // Next line
    controlsCursor.y += ImGui::GetTextLineHeightWithSpacing();
    return controlsCursor;
}


// copypasted from "imgui - node higlight" demo
void drawWaypointHighlights()
{
    using namespace math_utils;
    int mouseClosestNodeID = -1;
    
    int mouseClosestNodeDist = 999999;
    
    // this function draws inside the gridviewer
    ImDrawList@ drawlist = ImGui::GetWindowDrawList();
    for (int i = 0; i < int(npcWaypoints.length()); i++)
    {
        
        color col = cfgNodeColor;
        
        vector2 pos = gVertsViewer.localToScreenPos(npcWaypoints[i].position);
        
        // Draw the node
        if (npcWaypoints[i].selected)
        {
            // DOC: void AddRectFilled(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15)
            vector2 halfSize(cfgSelectedNodeSize/2.f, cfgSelectedNodeSize/2.f);
            drawlist.AddRectFilled(pos-halfSize, pos+halfSize, cfgSelectedNodeColor);
        }
        else
        {            
            //DOC: void ImDrawList::AddCircleFilled(const ImVec2& center, float radius, ImU32 col, int num_segments)
            drawlist.AddCircleFilled(pos, cfgNodeRadius, col, cfgNodeNumSegments);
        }
        
        // Accumulate mouse hover info
        int nodeDist = getMouseShortestDistance(game.getMouseScreenPosition(), pos);
        if (nodeDist < mouseClosestNodeDist)
        {
            mouseClosestNodeID = i;
            mouseClosestNodeDist = nodeDist;
        }
        
        // Draw mouse hover marker
        if (i == mouseHoveredNodeID)
        {
            // DOC: void AddCircle(const vector2&in center, float radius, const color&in col, int num_segments = 12, float thickness = 1.f)
            drawlist.AddCircle(pos, cfgNodeHoverRadius, cfgNodeHoverColor, cfgNodeNumSegments, cfgNodeHoverThickness);
        }
        
        
    }
    
    // Finalize hover detection
    mouseHoverDebugString="<DBG drawNodeHighlights> Mouse closest node: " + mouseClosestNodeID + " (dist:" + mouseClosestNodeDist + ")";
    if (mouseClosestNodeDist <= cfgNodeHoverMaxCursorDist)
    {
        mouseHoveredNodeID = mouseClosestNodeID;
    }
    else
    {
        mouseHoveredNodeID = -1;
    }
}

// #endregion







int npcIdCounter = 1;
class NpcCharacter
{
    Ogre::Entity@ ent;
    Ogre::SceneNode@ snode;
    string error;
    string uniqueName;
    vector3 currentPos;
    degree currentRot;
    int currentWp1;
    int currentWp2;
    float walkSpeed;
    float animSpeed;
    string dbgRotStr;
    
    NpcCharacter(int aInitialWp1, float aWalkSpd)
    {
        // we prepend the ScriptUnitID to avoid conflicts with leftover items from previous invocations of the script.
        uniqueName = "[NID " + thisScript + "] NpcCharacter"+npcIdCounter++ ;
        
        game.log("NpcCharacter() '"+uniqueName+"' is being set up");
        currentWp1 = aInitialWp1;
        this.findRoute(-1);
        
        walkSpeed = aWalkSpd;
        
        this.putToScene();
        // set initial  position - VERY IMPORTANT, the path progression is calculated from this.
        this.currentPos = npcWaypoints[this.currentWp1].position;
        
        // walking animation - enable once
        this.animSpeed = 1;
        Ogre::AnimationStateSet@ stateSet = this.ent.getAllAnimationStates();
        Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
        Ogre::AnimationState@ anim = stateDict["Walk"];
        anim.setEnabled(true);
    }
    
    void findRoute(int previousWp1)
    {
        // we have wp1 and want to select new wp2, preferably a different one than we came from
        int usableWp = -1;
        game.log("DBG findRoute() currentWp1="+this.currentWp1+", previousWp1="+previousWp1);
        for (int i = 0; i<int(npcPaths.length()); i++)
        {
            game.log("DBG findRoute() i="+i+" Wp1="+npcPaths[i].wp1+", Wp2="+npcPaths[i].wp2);
            int testWp = -1;
            if  (npcPaths[i].wp1 ==currentWp1 )
            {
                testWp = npcPaths[i].wp2;
            }
            else if ( npcPaths[i].wp2 == currentWp1)
            {
                testWp = npcPaths[i].wp1;
            }
            
            if (testWp != -1)
            {
                usableWp = testWp;
                game.log("DBG findRoute() found usable wp="+usableWp);
                if (previousWp1 !=  testWp)
                {
                    this.currentWp2 =  testWp;
                    break; // we have a new unused path!
                }
            }
        }
        
        // no unused path found - revert to the usable one
        this.currentWp2 = usableWp;
    }
    
    void advance(float dt)
    {
        float totalDist = npcWaypoints[this.currentWp1].position.distance(npcWaypoints[this.currentWp2].position);
        float curDist = npcWaypoints[this.currentWp1].position.distance(this.currentPos);
        float newDist = curDist + (this.walkSpeed * dt);
        while (newDist > totalDist)
        {
            // skip to new path
            newDist = newDist - totalDist;
            int prevWp1 = this.currentWp1;
            this.currentWp1 = this.currentWp2;
            this.findRoute(prevWp1);
            totalDist = npcWaypoints[this.currentWp1].position.distance(npcWaypoints[this.currentWp2].position);
        }
        
        // walk the rest and update character position
        float pctDist = newDist/totalDist;
        vector3 pos1 = npcWaypoints[this.currentWp1].position;
        vector3 pos2 = npcWaypoints[this.currentWp2].position;
        TerrainClass@ terrain = game.getTerrain();
        
        vector3 walkVec = (pos2-pos1);
        vector3 charaPos = pos1 + walkVec*pctDist;
        charaPos.y = terrain.getHeightAt(charaPos.x, charaPos.z);
        snode.setPosition(charaPos);
        this.currentPos = charaPos;
        
        // update character rotation
        
        snode.lookAt(pos2, Ogre::TS_WORLD);
        snode.yaw(radian(degree(90).valueRadians()), Ogre::TS_WORLD);
        
        // update character walking animation
        
        if (@this.ent != null)
        {
            Ogre::AnimationStateSet@ stateSet = this.ent.getAllAnimationStates();
            Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
            Ogre::AnimationState@ anim = stateDict["Walk"];
            
            float timepos = anim.getTimePosition() + (this.animSpeed * dt);
            
            
            anim.setTimePosition(timepos);
            
            
        }
    }
    
    void draw()
    {
using namespace math_utils;
        ImGui::PushID(this.uniqueName);
        vector3 pos1 = npcWaypoints[this.currentWp1].position;
        vector3 pos2 = npcWaypoints[this.currentWp2].position;
        vector3 walkVec = (pos2-pos1);
        ImGui::Text("DBG rot str: " + this.dbgRotStr+ " walkVec="+formatVector3(walkVec, 7, 2));
        ImGui::Text("DBG position:"+formatVector3(this.currentPos, 7, 2));
        ImGui::Text("DBG error: " + this.error);
        ImGui::Text("DBG wp1= " + this.currentWp1 + ", wp2="+this.currentWp2);
        float totalDist = npcWaypoints[this.currentWp1].position.distance(npcWaypoints[this.currentWp2].position);
        float curDist = npcWaypoints[this.currentWp1].position.distance(this.currentPos);
        float pctDist = curDist/totalDist;
        ImGui::Text("DBG curDist= " +curDist + ", totalDist="+totalDist+", pctDist="+pctDist+" walkSpeed="+walkSpeed);
        ImGui::Text("DBG pos1" + formatVector3(npcWaypoints[this.currentWp1].position, 7, 2));
        ImGui::Text("DBG pos2" + formatVector3(npcWaypoints[this.currentWp2].position, 7, 2));
        
        Ogre::AnimationStateSet@ stateSet = this.ent.getAllAnimationStates();
        Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
        Ogre::AnimationState@ anim = stateDict["Walk"];
        anim.setEnabled(true);
        float timepos = anim.getTimePosition() ;
        ImGui::Text("DBG walk anim timepos: "+ timepos + "(anim speed: "+this.animSpeed+")");
        
        if (@this.ent != null && @this.ent.getAllAnimationStates() != null)
        {
            this.drawAnimationControls(this.ent.getAllAnimationStates());
        }
        ImGui::PopID(); //(this.uniqueName);
        
    }
    
    private void drawAnimationControls(Ogre::AnimationStateSet@ stateSet)
    {
        Ogre::AnimationStateDict@ stateDict = stateSet.getAnimationStates(); 
        array<string> stateNames = stateDict.getKeys();
        for (uint i = 0; i < stateDict.getSize(); i++)
        {
            
            
            Ogre::AnimationState@ anim = stateDict[stateNames[i]];
            ImGui::PushID(anim.getAnimationName());
            ImGui::BulletText('"' + anim.getAnimationName() + '"');
            
            
            
            
            ImGui::SameLine();
            bool enabled = anim.getEnabled();
            if (ImGui::Checkbox("Enabled", enabled))
            {
                anim.setEnabled(enabled);
            }
            
            if (anim.getEnabled())
            {
                ImGui::SameLine();
                float weight = anim.getWeight();
                ImGui::SetNextItemWidth(75.f);
                if (ImGui::SliderFloat("BlendWeight", weight, 0.f, 1.f))
                {
                    anim.setWeight(weight);
                }
                
                ImGui::Dummy(vector2(25.f, 0.f)); // Indent
                ImGui::SameLine();
                float timepos = anim.getTimePosition();
                if (ImGui::SliderFloat("TimePos", timepos, 0.f, anim.getLength()))
                {
                    anim.setTimePosition(timepos);
                }
            }
            
            ImGui::PopID(); // anim.getAnimationName()
        }
    }
    
    void putToScene()
    {
        if (@this.ent == null)
        {      
            game.log("DBG putToScene() '"+uniqueName+"'"); 
            // Load the rorbot
            @this.ent = game.getSceneManager().createEntity(uniqueName+"-entity", "character.mesh" );
            if (@this.ent == null)
            {
                error = "Could not load 'character.mesh'";
                return;
            }
            
            // Put to scene
            @this.snode = game.getSceneManager().getRootSceneNode().createChildSceneNode(uniqueName+"-node");
            this.snode.attachObject(this.ent);
            // 'character.mesh' has wrong scale, the game scales it down to <0.02, 0.02, 0.02>
            this.snode.setScale(vector3(0.02, 0.02, 0.02));
            // dress up directly with the shared material, we won't be changing shirt color
            this.ent.setMaterialName("tracks/character");
        }
    }
    
    void moveTo(vector3 pos, degree rot)
    {
        this.currentPos = pos;
        this.currentRot = rot;
        
        this.snode.setPosition(currentPos);
        this.snode.yaw(currentRot.valueDegrees());    
    }
    
    void removeFromScene()
    {
        if (@this.ent != null)
        {
            this.ent.detachFromParent();
            game.getSceneManager().destroyEntity(this.ent);
            @this.ent = null;
            
            game.getSceneManager().destroySceneNode(this.snode);
            @this.snode = null;
            
            
        }
        
    }
};
// #region Walker management
array<NpcCharacter@> npcCharacters;
void releaseWalker()
{
    if (npcPaths.length()<1) 
    {
        game.log("releaseWalker(): There must be at least one path to add NPC pedestrian.");
    }
    else     if (lastNumSelected < 1)
    {
        game.log("releaseWalker(): A starting waypoint must be selected. In case of multi selecion, the first one is used."); 
    }
    else
    {
        // check the selected waypoint is connected to a path!
        // each-to-each (i=front, j=rear)
        int numPathsBefore =int(npcPaths.length());
        for (int i=0; i<int(npcWaypoints.length()); i++)
        {
            if (!npcWaypoints[i].selected)
            {
                continue;
            }
            for (int j=i; j<int(npcWaypoints.length()); j++)
            {
            if (i==j) {continue;}
                if (            pathExistsBetween(i,j))
                {
                    // found usable path! add the walker.
                    float speed = 1;
                    npcCharacters.insertLast(NpcCharacter(i, speed));
                    return;
                }
                else
                {
                    game.log("DBG releaseWalker(): no path between selected" + i + " and tested "+ j);
                }
            }
            
        }
        game.log("releaseWalker(): No usable path found among the selected waypoints");
    }
}

void killWalker(int iKill)
{
    if (iKill < 0 || iKill >= int( npcCharacters.length()))
    {
        gErrorStr = "cannot delete npcCahcarcter "+iKill+", count is "+npcCharacters.length();
        return;
    }
    npcCharacters[iKill].removeFromScene();
    npcCharacters.removeAt(iKill);
    
}

void advanceWalkers(float dt)
{
    // advance the NPC characters
    int iKill = -1;
    for (int i=0; i<int(npcCharacters.length()); i++)
    {
        npcCharacters[i].advance(dt);
        if (ImGui::CollapsingHeader(npcCharacters[i].uniqueName))
        {
            if (ImGui::SmallButton("Kill this walker"))
            {
                iKill = i;
            }
            npcCharacters[i].draw(); // draw UI for animations.
        }
    }
    if (iKill != -1)
    {
        killWalker(iKill);
    }
}
//#endregion

// #region Game callbacks

void main()
{
    setupPathEditor();
    game.registerForEvent(SE_ANGELSCRIPT_MANIPULATIONS );  // we want to clean up after ourselves at exit
}

void frameStep(float dt)
{
    ImGui::Begin("Pedestrians example", closeBtnHandler.windowOpen, 0);
    closeBtnHandler.draw();
    if (gErrorStr == "")
    {
        drawPathEditor();
        advanceWalkers(dt);
    }
    else
    {
        ImGui::TextDisabled("E R R O R !!");
        ImGui::TextColored(color(1, 0.2, 0, 1), gErrorStr);
    }
    ImGui::End();    
}

void eventCallbackEx(scriptEvents ev, // Invoked by the game when a registered event is triggered
int arg1, int arg2ex, int arg3ex, int arg4ex,
string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    // when the script is exiting, we want to clean up after ourselves
    //ASMANIP_SCRIPT_UNLOADING
    //Args: #2 ScriptUnitId_t, #3 RoR::ScriptCategory, #4 unused, #5 filename. 
    if (ev == SE_ANGELSCRIPT_MANIPULATIONS 
    && arg1 == ASMANIP_SCRIPT_UNLOADING
    && arg2ex == thisScript)
    {
        for (int i=0; i<int(npcCharacters.length()); i++)
        {
            npcCharacters[i].removeFromScene();
            
        }
    }
    
    
    
}

// #endregion

