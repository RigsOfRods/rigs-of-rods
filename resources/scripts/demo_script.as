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
     * Load and write text files in the resource system.
     * Inspect loaded sounds and soundscript templates, and of course play sounds!
     * Post messages to game's main message queue, performing almost any operation.
     
    There are several ways of invoking a script:
     1. From in-game console (hotkey ~), say 'loadscript <filename>'
     2. By using command line parameter '-runscript <filename>'.
        You can use this command multiple times at once.
     3. By setting 'app_custom_scripts' in RoR.cfg to a comma-separated list
        of filenames. Spaces in filename are acceptable.
     4. By defining it with terrain, see terrn2 fileformat, section '[Scripts]':
        https://docs.rigsofrods.org/terrain-creation/terrn2-subsystem/.
        This is the classic old method, used for i.e. races.        
    
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
CVarClass@  g_sys_cache_dir = console.cVarFind("sys_cache_dir");
CVarClass@  g_sim_state = console.cVarFind("sim_state"); // 0=off, 1=running, 2=paused, 3=terrain editor, see SimState in Application.h
CVarClass@  g_mp_state = console.cVarFind("mp_state"); // 0=disabled, 1=connecting, 2=connected, see MpState in Application.h
CVarClass@  g_io_arcade_controls = console.cVarFind("io_arcade_controls"); // bool
GenericDocumentClass@ g_displayed_document = null;
string g_displayed_doc_filename;
array<string> g_terrain_tobj_files;
SoundScriptInstanceClass@ g_playing_soundscript = null;
SoundClass@ g_playing_sound = null;
bool g_sound_follows_player = true;
string g_demofile_data;

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
        
        ImGui::Separator();
        drawAudioButtons();
        
        ImGui::Separator();
        drawTextResourceButtons();        
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
                
            drawActorAngles(actor);
            
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
        ImGui::Separator();
        drawAIButtons();
        
        ImGui::Separator();
        drawAudioButtons();
        
        ImGui::Separator();
        drawTextResourceButtons();
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

vector3 detectPlayerPosition()
{
    if (g_app_state.getInt() == 2) // simulation
    {

        // get current pos
        vector3 pos;
        BeamClass@ actor = game.getCurrentTruck();
        if (@actor != null)
            pos = actor.getVehiclePosition();
        else
            pos = game.getPersonPosition();

        return pos;
            
    }
    else // main menu
    {
        return vector3(0,0,0);
    }
}

void drawAudioButtons()
{
    ImGui::PushID("AudioTest");
    ImGui::TextDisabled("Audio API test");
    
    if (g_app_state.getInt() == 1) // main menu
    {
        ImGui::TextDisabled("You are in main menu - spatial (3D) audio is off");
    }
    else if (g_app_state.getInt() == 2) // simulation
    {
        ImGui::TextDisabled("You are in simulation - spatial (3D) audio is on");
        ImGui::Checkbox("Sound follows player", g_sound_follows_player);

        // Update sound positions
        if (g_sound_follows_player)
        {
            vector3 pos = detectPlayerPosition();
            if (@g_playing_sound != null)
                g_playing_sound.setPosition(pos);
            if (@g_playing_soundscript != null)
                g_playing_soundscript.setPosition(pos);
        }
    }
    
    array<SoundScriptTemplateClass@>@ templates = game.getAllSoundScriptTemplates();
    string templates_title = "Sound script templates (" + templates.length() + ")";
    if (ImGui::CollapsingHeader(templates_title))
    {
        ImGui::PushID("templates");
        
        for (uint i = 0; i < templates.length(); i++)
        {
            ImGui::PushID(i);
            
            SoundScriptTemplateClass@ template = game.getSoundScriptTemplate(templates[i].getName()); // Look up again by name, just to test the API
            
            if (@template != null)
            {
                ImGui::Text(template.getName());
                if (template.isBaseTemplate())
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled(" [base]");
                }
                ImGui::SameLine();

                if (@g_playing_soundscript == null)
                {
                    if (ImGui::Button("Play"))
                    {
                        @g_playing_soundscript = game.createSoundScriptInstance(template.getName());
                        if (@g_playing_soundscript != null)
                        {
                            g_playing_soundscript.setPosition(detectPlayerPosition());
                            g_playing_soundscript.start();
                        }
                        else
                        {
                            game.log("Demo script: could not create sound script instance from template '" + template.getName() + "'");
                        }
                    }
                }
                else
                {
                    if (ImGui::Button("Stop"))
                    {
                        g_playing_soundscript.kill();
                        @g_playing_soundscript = null;
                    }
                }
            }
            else
            {
                ImGui::Text("(Lookup failed for template "+i+"/"+templates.length()+")");
            }
            
            ImGui::PopID(); // i
        }
        
        ImGui::PopID(); // "templates"
    }
    
    array<SoundScriptInstanceClass@>@ instances = game.getAllSoundScriptInstances();
    string instances_title = "Sound script instances (" + instances.length() + ")";
    if (ImGui::CollapsingHeader(instances_title))
    {
        ImGui::PushID("instances");
    
        for (uint i = 0; i < instances.length(); i++)
        {
            ImGui::PushID(i);
        
            SoundScriptInstanceClass@ instance = instances[i];
            
            ImGui::Text(instance.getInstanceName());
            ImGui::SameLine();
            ImGui::TextDisabled("(show tooltip with details)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                drawSoundScriptInstanceDiagPanel(instance);
                ImGui::EndTooltip();
            }
            
            ImGui::PopID(); // i
        }
        
        ImGui::PopID(); // "instances"
    }
    
    ImGui::TextDisabled("Some builtin sounds");

    drawWavPreviewBulletButton("default_horn.wav");
    drawWavPreviewBulletButton("default_police.wav");
    drawWavPreviewBulletButton("default_pump.wav");
    drawWavPreviewBulletButton("default_shift.wav");
    drawWavPreviewBulletButton("default_starter.wav");
    
    ImGui::PopID(); // "AudioTest"
}

void drawWavPreviewBulletButton(string wav_file)
{
    ImGui::PushID(wav_file);

    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::Text(wav_file);
    ImGui::SameLine();
    if (@g_playing_sound == null)
    {
        if (ImGui::Button("Play loop"))
        {        
            @g_playing_sound = game.createSoundFromResource(wav_file);
            g_playing_sound.setEnabled(true);
            g_playing_sound.setGain(1.f);
            g_playing_sound.setLoop(true);
            g_playing_sound.setPosition(detectPlayerPosition());
            g_playing_sound.play();
            game.log("Demo script: playing file " + wav_file);
        }
    }
    else
    {
        if (ImGui::Button("Stop"))
        {
            g_playing_sound.stop();
            @g_playing_sound = null;
            game.log("Demo script: stopping file " + wav_file);
        }
    }
    
    ImGui::PopID(); // wav_file
}

void drawSoundObjectDiag(SoundClass@ snd)
{
    string txt 
        = "\t enabled:"+snd.getEnabled()+", playing:"+snd.isPlaying()
        +"\n\t audibility:"+snd.getAudibility()
        +"\n\t gain:"+snd.getGain() +", pitch:"+snd.getPitch()
        +"\n\t loop:"+snd.getLoop()
        +"\n\t currentHardwareIndex:"+snd.getCurrentHardwareIndex()
        +"\n\t OpenAL buffer ID:"+snd.getBuffer()
        +"\n\t position: X="+snd.getPosition().x+" Y="+snd.getPosition().y+" Z="+snd.getPosition().z
        +"\n\t velocity: X="+snd.getVelocity().x+" Y="+snd.getVelocity().y+" Z="+snd.getVelocity().z;
    ImGui::Text(txt);
}

void drawSoundScriptInstanceDiagPanel(SoundScriptInstanceClass@ instance)
{
    SoundScriptTemplateClass@ template = instance.getTemplate();

    // START sound
    SoundClass@ startSnd = instance.getStartSound();
    if (@startSnd != null)
    {
        ImGui::Text("START sound: '" + template.getStartSoundName() + "' (pitchgain: "+instance.getStartSoundPitchgain()+")");
        drawSoundObjectDiag(startSnd);
    }
    else
    {
        ImGui::TextDisabled("[no START sound]");
    }
    
    // SOUNDS (running)
    int numSounds = template.getNumSounds();
    ImGui::Text("SOUNDS (count: " + numSounds + ")");
    for (int i = 0; i < numSounds; i++)
    {
        SoundClass@ snd = instance.getSound(i);
        ImGui::Text("SOUND: '" + template.getSoundName(i) + "' (pitchgain: "+instance.getSoundPitchgain(i)+")");
        drawSoundObjectDiag(snd);
    }
    
    // STOP sound
    SoundClass@ stopSnd = instance.getStopSound();
    if (@stopSnd != null)
    {
        ImGui::Text("STOP sound: '" + template.getStopSoundName() + "' (pitchgain: "+instance.getStopSoundPitchgain()+")");
        drawSoundObjectDiag(stopSnd);
    }
    else
    {
        ImGui::TextDisabled("[no STOP sound]");
    }    
}

void drawAIButtons()
{
    ImGui::TextDisabled("AI script test");
    if (ImGui::Button("Start DAF semitruck in follow-mode"))
    {
        // the filename
        // first parameter is index - only use 0/1 for drag race and crash modes. Otherwise use 0.
        game.setAIVehicleName(0, "b6b0UID-semi.truck");
        
        // 0) Normal driving mode
        // 1) Race
        // 2) Drag Race
        // 3) Crash driving mode
        // 4) Chase the player mode
        game.setAIMode(4);
        
        // define the start position by inserting initial waypoint.
        game.addWaypoint(game.getPersonPosition() + vector3(6, 0, 6)); // 6 meters away from player
        
        // define the start direction by inserting another waypoint
        game.addWaypoint(game.getPersonPosition()); // look at player!
        
        // Request loading the AI script (asynchronously) - it will spawn the vehicle.
        // WARNING: this doesn't save off the setup values above - you can still modify them below and change what the AI will do!
        //          If you want to launch multiple AIs in sequence, register for SE_GENERIC_NEW_TRUCK event - when it arrives, it's safe to setup and launch new AI script.
        game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED, { {"filename", "AI.as"} });
    }
}

void drawTextResourceButtons()
{    
    ImGui::TextDisabled("Text file test: file 'demofile.txt' in group 'Cache'");
    ImGui::TextDisabled("Location: " + g_sys_cache_dir.getStr());
    
    // The 'exists?' line
    bool exists = game.checkResourceExists("demofile.txt", "Cache");
    if (exists)
    {
        ImGui::Text("File exists.");
        ImGui::SameLine();
        if (ImGui::Button("(re)load"))
        {
            g_demofile_data = game.loadTextResourceAsString("demofile.txt", "Cache");
        }
    }
    else
    {
        ImGui::Text("File does not exist yet, create it below.");
    }
    
    // The text editor part
    ImGui::InputText("demofile text", g_demofile_data);
    if (ImGui::Button("(over)write"))
    {
        game.createTextResourceFromString(g_demofile_data, "demofile.txt", "Cache", /*overwrite:*/true);
    }
    ImGui::SameLine();
    if (ImGui::Button("delete"))
    {
        game.deleteResource("demofile.txt", "Cache");
        g_demofile_data = "";
    }
}

string formatVector3(vector3 val, int total, int frac)
{
    return "X:" + formatFloat(val.x, "", total, frac)
        + " Y:" + formatFloat(val.y, "", total, frac)
        + " Z:" + formatFloat(val.z, "", total, frac);
}

void drawActorAngles(BeamClass@ actor)
{
    if (ImGui::CollapsingHeader("Actor positions/angles test"))
    {
        ImGui::Text("getPosition(): [vector3] " + formatVector3(actor.getPosition(), 6,2));
        //ImGui::Text("getRotation(): [float] " + formatFloat(actor.getRotation(), "", 6,2)); // returns valid yaw in radians, but prefer using `.getOrientation.getYaw()`
        ImGui::Text("getSpeed(): [float] " + formatFloat(actor.getSpeed(), "", 6,2));
        ImGui::Text("getOrientation(): [quaternion]");
        ImGui::Text("  .getYaw(): [radian] "   + formatFloat(actor.getOrientation().getYaw().valueRadians(), "", 6,2)   
            + " (degrees: " + formatFloat(actor.getOrientation().getYaw().valueDegrees(), "", 6,2) + ")");
        ImGui::Text("  .getPitch(): [radian] " + formatFloat(actor.getOrientation().getPitch().valueRadians(), "", 6,2) 
            + " (degrees: " + formatFloat(actor.getOrientation().getPitch().valueDegrees(), "", 6,2) + ")");
        ImGui::Text("  .getRoll(): [radian] "  + formatFloat(actor.getOrientation().getRoll().valueRadians(), "", 6,2)  
            + " (degrees: " + formatFloat(actor.getOrientation().getRoll().valueDegrees(), "", 6,2) + ")");
    }
}
