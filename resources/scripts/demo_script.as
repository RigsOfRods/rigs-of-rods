/*
    ---------------------------------------------------------------------------
                  Project Rigs of Rods (www.rigsofrods.org)
                            
                                DEMO SCRIPT
                                
    This program showcases all the various things you can do using scripting:
     * Use DearIMGUI to draw UI of any kind, including diagnostic views.
     * Collect and show stats (i.e. frame count, total time)
     * Read/Write cvars (RoR.cfg values, cli args, game state...)
     * View and update game state (current vehicle...)
     * Parse and display definition files with syntax highlighting.
     
    There are 3 ways of invoking a script:
     1. By defining it with terrain, see terrn2 fileformat, section '[Scripts]':
        https://docs.rigsofrods.org/terrain-creation/terrn2-subsystem/.
        This is the classic old method, used for i.e. races.
     2. By using command line parameter '-runscript <filename>'.
        You can use this command multiple times at once. Added in 2022.
     3. By setting 'app_custom_scripts' in RoR.cfg to a comma-separated list
        of filenames. Spaces in filename are acceptable. Added in 2022.
    
    For introduction to game events, read
    https://docs.rigsofrods.org/terrain-creation/scripting/.
    
    Scripting documentation:
    https://developer.rigsofrods.org/d4/d07/group___script2_game.html
     
    ---------------------------------------------------------------------------
*/

/*
    ---------------------------------------------------------------------------
    Global variables
*/
int         g_total_frames = 0;
float       g_total_seconds = 0;
CVarClass@  g_app_state = console.cVarFind("app_state"); // 0=bootstrap, 1=main menu, 2=simulation, see AppState in Application.h
CVarClass@  g_sim_state = console.cVarFind("sim_state"); // 0=off, 1=running, 2=paused, 3=terrain editor, see SimState in Application.h
CVarClass@  g_mp_state = console.cVarFind("mp_state"); // 0=disabled, 1=connecting, 2=connected, see MpState in Application.h
CVarClass@  g_io_arcade_controls = console.cVarFind("io_arcade_controls"); // bool
GenericDocumentClass@ g_displayed_document = null;
string g_displayed_doc_filename;
array<string> g_terrain_tobj_files;

/*
    ---------------------------------------------------------------------------
    Script setup function - invoked once when script is loaded.
*/
void main()
{
    log("Hello Rigs of Rods!");
}

/*
    ---------------------------------------------------------------------------
    Script update function - invoked once every rendered frame,
    with elapsed time (delta time, in seconds) as parameter.
*/
void frameStep(float dt)
{
    // Open demo window
    ImGui::Begin("Demo Script", /*open:*/true, ImGuiWindowFlags_AlwaysAutoResize);
    
    // show some stats
    ImGui::Text("Total frames: " + g_total_frames);
    ImGui::Text("Total time: " + int(g_total_seconds / 60) + "min, " 
                               + int(g_total_seconds % 60) + "sec");
    
    // Show some game context
    if (g_app_state.getInt() == 1) // main menu
    {
        drawMainMenuPanel();
    }
    else if (g_app_state.getInt() == 2) // simulation
    {
        if (g_mp_state.getInt() == 2)
        {
            ImGui::Text("Game state: multiplayer");
        }
        else
        {
            ImGui::Text("Game state: single player");
        }
        
        if (g_sim_state.getInt() == 2)
        {
            ImGui::SameLine();
            ImGui::Text("(paused)");
        }
        else if (g_sim_state.getInt() == 3)
        {
            ImGui::SameLine();
            ImGui::Text("(terrain edit)");
        }
        
        drawTerrainButtons();
        
        ImGui::TextDisabled("Camera controls:");
        ImGui::Text("Change camera: " + inputs.getEventCommandTrimmed(EV_CAMERA_CHANGE));
        ImGui::Text("Toggle free camera: " + inputs.getEventCommandTrimmed(EV_CAMERA_FREE_MODE));
        ImGui::Text("Toggle fixed camera: " + inputs.getEventCommandTrimmed(EV_CAMERA_FREE_MODE_FIX));              
        
        BeamClass@ actor = game.getCurrentTruck();
        if (@actor != null)
        {
            // Actor name and "View document" button
            ImGui::PushID("actor");
            ImGui::AlignTextToFramePadding();
            ImGui::Text("You are driving " + actor.getTruckName());
            ImGui::SameLine();
            if (@g_displayed_document == null)
            {
                if (ImGui::Button("View document"))
                {
                    GenericDocumentClass@ doc = GenericDocumentClass();
                    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
                              | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
                              | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE
                              | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
                              | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;
                    if (doc.loadFromResource(actor.getTruckFileName(), actor.getTruckFileResourceGroup(), flags))
                    {
                        @g_displayed_document = @doc;
                        g_displayed_doc_filename = actor.getTruckFileName();
                    }
                }
            }
            else
            {
                if (ImGui::Button("Close document"))
                {
                    @g_displayed_document = null;
                    g_displayed_doc_filename = "";
                }
            }
            ImGui::PopID(); //"actor"
            
            ImGui::TextDisabled("Vehicle controls:");

            ImGui::Text("Accelerate/Brake: "
                + inputs.getEventCommandTrimmed(EV_TRUCK_ACCELERATE) + "/"
                + inputs.getEventCommandTrimmed(EV_TRUCK_BRAKE));
            if (g_io_arcade_controls.getBool() == true)
            {
                ImGui::Text("Arcade controls are enabled (?)");
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("'brake' key also accelerates in reverse.");
                    ImGui::Text("You can change the setting in main menu / settings / gameplay.");
                    ImGui::EndTooltip();
                }
            }
            else
            {
                ImGui::Text("Arcade controls are disabled (?)");
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("'brake' key only brakes, to accelerate in reverse use 'accelerate' key.");
                    ImGui::Text("You can change the setting in main menu / settings / gameplay.");
                    ImGui::EndTooltip();
                }
            }
            
            ImGui::Text("Steer left/right: "
                + inputs.getEventCommandTrimmed(EV_TRUCK_STEER_LEFT) + "/"
                + inputs.getEventCommandTrimmed(EV_TRUCK_STEER_RIGHT));            
            
        }
        else
        {
            ImGui::Text("You are on foot");
            ImGui::TextDisabled("Character controls:");
            ImGui::Text("Forward/Backward: "
                + inputs.getEventCommandTrimmed(EV_CHARACTER_FORWARD) + "/"
                + inputs.getEventCommandTrimmed(EV_CHARACTER_BACKWARDS));
            ImGui::Text("Turn left/right: "
                + inputs.getEventCommandTrimmed(EV_CHARACTER_LEFT) + "/"
                + inputs.getEventCommandTrimmed(EV_CHARACTER_RIGHT));
            ImGui::Text("Run: " + inputs.getEventCommandTrimmed(EV_CHARACTER_RUN));
        }
        
    }
    
    // End window
    ImGui::End();

    // Update global counters
    g_total_frames++;
    g_total_seconds += dt;
    
    // Draw document window
    if (@g_displayed_document != null)
    {
        drawDocumentWindow();
    }
}

void drawTerrainButtons()
{
    // Terrain name (with "view document" button)
    ImGui::PushID("terrn");
    TerrainClass@ terrain = game.getTerrain();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Terrain: " + terrain.getTerrainName());
    ImGui::SameLine();
    if (@g_displayed_document == null)
    {
        if (ImGui::Button("View document"))
        {
            GenericDocumentClass@ doc = GenericDocumentClass();
            int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
                      | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
                      | GENERIC_DOCUMENT_OPTION_ALLOW_HASH_COMMENTS
                      | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_EQUALS
                      | GENERIC_DOCUMENT_OPTION_ALLOW_BRACED_KEYWORDS;
            if (doc.loadFromResource(terrain.getTerrainFileName(), terrain.getTerrainFileResourceGroup(), flags))
            {
                @g_displayed_document = @doc;
                g_displayed_doc_filename = terrain.getTerrainFileName();
                
                // Fetch TOBJ filenames
                if (g_terrain_tobj_files.length() == 0)
                {
                    GenericDocReaderClass@ reader = GenericDocReaderClass(doc);
                    bool in_section_objects = false;
                    while (!reader.endOfFile())
                    {
                        if (reader.tokenType() == TOKEN_TYPE_KEYWORD && reader.getTokKeyword().substr(0, 1) == "[")
                        {
                            in_section_objects = (reader.getTokKeyword() == '[Objects]');
                        }
                        else if (reader.tokenType() == TOKEN_TYPE_STRING && in_section_objects)
                        {
                            // Note: in GenericDocument, a text on line start is always a KEYWORD token,
                            // but KEYWORDs must not contain special characters,
                            // so file names always decay to strings because of '.'.                    
                            g_terrain_tobj_files.insertLast(reader.getTokString());
                        }
                        reader.moveNext();
                    }
                }
            }
        }
    }
    else
    {
        if (ImGui::Button("Close document"))
        {
            @g_displayed_document = null;
            g_displayed_doc_filename = "";
        }
    }

    // TOBJ files
    ImGui::PushID("tobj");
    for (uint i = 0; i < g_terrain_tobj_files.length(); i++)
    {
        ImGui::PushID(i);
        ImGui::AlignTextToFramePadding();
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::Text(g_terrain_tobj_files[i]);
        ImGui::SameLine();
        if (@g_displayed_document == null)
        {
            if (ImGui::Button("View document"))
            {
                GenericDocumentClass@ doc = GenericDocumentClass();
                int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
                          | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS;
                if (doc.loadFromResource(g_terrain_tobj_files[i], terrain.getTerrainFileResourceGroup(), flags))
                {
                    @g_displayed_document = @doc;   
                    g_displayed_doc_filename = g_terrain_tobj_files[i];
                }
            }
        }
        else
        {
            if (ImGui::Button("Close document"))
            {
                @g_displayed_document = null;
                g_displayed_doc_filename = "";
            }
        }
        ImGui::PopID(); // i
    }
    ImGui::PopID(); //"tobj"
    
    ImGui::PopID(); //"terrn"
}

void drawDocumentWindow()
{
    ImGui::PushID("document view");
    string caption = "Document view (" + g_displayed_doc_filename + ")";
    ImGui::Begin(caption, /*open:*/true, /*flags:*/0);

    GenericDocReaderClass reader(g_displayed_document);
    while (!reader.endOfFile())
    {
        switch (reader.tokenType())
        {
        // These tokens are always at start of line
        case TOKEN_TYPE_KEYWORD:
            ImGui::TextColored(color(1.f, 1.f, 0.f, 1.f), reader.getTokKeyword());
            break;
        case TOKEN_TYPE_COMMENT:
            ImGui::TextDisabled(";" + reader.getTokComment());
            break;
            
        // Linebreak is implicit in DearIMGUI, no action needed
        case TOKEN_TYPE_LINEBREAK:
            break;

        // Other tokens come anywhere - delimiting logic is needed
        default:
            if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
            {
                ImGui::SameLine();
                string delimiter = (reader.tokenType(-1) == TOKEN_TYPE_KEYWORD) ? " " : ", ";
                ImGui::Text(delimiter);
                ImGui::SameLine();
            }

            switch (reader.tokenType())
            {
            case TOKEN_TYPE_STRING:
                ImGui::TextColored(color(0.f, 1.f, 1.f, 1.f), "\"" + reader.getTokString() + "\"");
                break;
            case TOKEN_TYPE_NUMBER:
                ImGui::Text("" + reader.getTokFloat());
                break;
            case TOKEN_TYPE_BOOL:
                ImGui::TextColored(color(1.f, 0.f, 1.f, 1.f), ""+reader.getTokBool());
                break;
            }
        }
        
        reader.moveNext();
    }
    
    ImGui::End();
    ImGui::PopID(); //"document view"
}

void drawMainMenuPanel()
{
    ImGui::Text("Game state: main menu");
    ImGui::Text("Pro tip: Press '"
        + inputs.getEventCommandTrimmed(EV_COMMON_CONSOLE_TOGGLE)
        + "' to open console anytime.");
        
    // Test message queue
    if (ImGui::Button("Launch simple test terrain"))
    {
        game.pushMessage(MSG_SIM_LOAD_TERRN_REQUESTED, {{'filename', 'simple2.terrn2'}});
    }
        
    // Reset simulation data
    @g_displayed_document = null;
    g_displayed_doc_filename = "";
    g_terrain_tobj_files.removeRange(0, g_terrain_tobj_files.length());
}
