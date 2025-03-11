/// \title IMGUI-based editor of tokenized GenericDocument
/// \opens the terrain files (.terrn2, .tobj)
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// genericDoc editor
#include "genericdoc_utils.as"
genericdoc_utils::GenericDocEditor gdEditor;

int hoveredTokenPos = -1;
int focusedTokenPos = -1;
string errorString;
// Document window state
GenericDocumentClass@ g_displayed_document = null;
string g_displayed_doc_filename;
array<string> g_terrain_tobj_files;



// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    
    
    string caption = "GenericDocument editor";
    int flags = ImGuiWindowFlags_MenuBar;
    if (ImGui::Begin(caption, closeBtnHandler.windowOpen, flags))
    {
        closeBtnHandler.draw();
        
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("Open document"))
            {
                ImGui::TextDisabled("rig-definition file (aka truck file):");
                drawTruckDocumentControls();
                ImGui::Separator();
                
                ImGui::TextDisabled("Terrain documents (terrn2, tobj)");
                drawTerrainButtons();
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
            
        }
        
        
        if (@g_displayed_document != null)
        {
            if (@g_displayed_document != @gdEditor.displayedDocument)
            {
                gdEditor.setDocument(g_displayed_document, g_displayed_doc_filename);
            }
            
            vector2 size = ImGui::GetWindowSize() - vector2(20, 150);
            ImGui::BeginChild("docBody", size);
            gdEditor.drawDocumentBody();
            ImGui::EndChild();
            
            
            ImGui::Separator();
            
            gdEditor.drawTokenEditPanel();
            
        }
        
        ImGui::End();
        
    }
}



//#region TOBJ viewing
void loadTobjDocument(uint i)
{
    TerrainClass@ terrain = game.getTerrain();
    if (@terrain == null)
    {
        game.log("loadTobjDocument(): no terrain loaded, nothing to do!");
        return;
    }
    GenericDocumentClass@ doc = GenericDocumentClass();
    if (doc.loadFromResource(g_terrain_tobj_files[i], terrain.getTerrainFileResourceGroup(), genericdoc_utils::FLAGSET_TOBJ))
    {
        @g_displayed_document = @doc;   
        g_displayed_doc_filename = g_terrain_tobj_files[i];
    }
}

void drawDiscoveredTobjFiles()
{
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
                loadTobjDocument(i);
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
}
//#endregion

//#region Terrn doc viewing
void loadTerrn2Document()
{
    TerrainClass@ terrain = game.getTerrain();
    if (@terrain == null)
    {
        game.log("loadTerrn2Document(): no terrain loaded, nothing to do!");
        return;
    }
    GenericDocumentClass@ doc = GenericDocumentClass();
    
    if (!doc.loadFromResource(terrain.getTerrainFileName(), terrain.getTerrainFileResourceGroup(), genericdoc_utils::FLAGSET_TERRN2))
    {
        game.log("loadTerrn2Document() - could not load file");
        return;
    }
    
    @g_displayed_document = @doc;
    g_displayed_doc_filename = terrain.getTerrainFileName();
    
    // Fetch TOBJ filenames
    if (g_terrain_tobj_files.length() == 0)
    {
        GenericDocContextClass@ reader = GenericDocContextClass(doc);
        bool in_section_objects = false;
        bool in_section_races = false;
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
            
            loadTerrn2Document();
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
    
    ImGui::TextDisabled("[Objects]");
    drawDiscoveredTobjFiles();
    
    
    ImGui::PopID(); //"terrn"
}

//#endregion

//#region Truck file format viewing
void loadTruckDocument()
{
    BeamClass@ actor = game.getCurrentTruck();
    if (@actor == null)
    {
        game.log("loadTruckDocument(): you are on foot, nothing to do!");
        return;
    }
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
            
            if (doc.loadFromResource(actor.getTruckFileName(), actor.getTruckFileResourceGroup(), genericdoc_utils::FLAGSET_TRUCK))
            {
                @g_displayed_document = @doc;
                g_displayed_doc_filename = actor.getTruckFileName();
            }
        }
    }
}

void drawTruckDocumentControls()
{
    
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
                
                if (doc.loadFromResource(actor.getTruckFileName(), actor.getTruckFileResourceGroup(), genericdoc_utils::FLAGSET_TRUCK))
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
    }
    else
    {
        ImGui::Text("You are on foot. Spawn a vehicle to access it's definition file.");
    }
    
}
//#endregion



