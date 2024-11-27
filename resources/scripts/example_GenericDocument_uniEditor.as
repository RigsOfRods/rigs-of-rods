/// \title IMGUI-based editor of tokenized GenericDocument
/// \opens the terrain files (.terrn2, .tobj)
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;


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
            vector2 size = ImGui::GetWindowSize() - vector2(20, 150);
            ImGui::BeginChild("docBody", size);
            drawDocumentBody();
            ImGui::EndChild();
            
            
            ImGui::Separator();
            
            drawTokenEditPanel();
            
        }
        
        ImGui::End();
        
    }
}


//#region Terrn+tobj doc viewing
void loadTobjDocument(uint i)
{
    TerrainClass@ terrain = game.getTerrain();
    if (@terrain == null)
    {
        game.log("loadTobjDocument(): no terrain loaded, nothing to do!");
        return;
    }
    GenericDocumentClass@ doc = GenericDocumentClass();
    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS;
    if (doc.loadFromResource(g_terrain_tobj_files[i], terrain.getTerrainFileResourceGroup(), flags))
    {
        @g_displayed_document = @doc;   
        g_displayed_doc_filename = g_terrain_tobj_files[i];
    }
}

void loadTerrn2Document()
{
    TerrainClass@ terrain = game.getTerrain();
    if (@terrain == null)
    {
        game.log("loadTerrn2Document(): no terrain loaded, nothing to do!");
        return;
    }
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
            GenericDocContextClass@ reader = GenericDocContextClass(doc);
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
    }
    else
    {
        ImGui::Text("You are on foot. Spawn a vehicle to access it's definition file.");
    }
    
}
//#endregion

//#region Document view


void drawDocumentBody()
{
    ImGui::PushID("docBody");
    bool hover_found = false;
    
    GenericDocContextClass reader(g_displayed_document);
    while (!reader.endOfFile())
    {
        
        
        switch (reader.tokenType())
        {
            // These tokens are always at start of line
            case TOKEN_TYPE_KEYWORD:
            ImGui::TextColored(tokenColor(reader), reader.getTokKeyword());
            break;
            case TOKEN_TYPE_COMMENT:
            ImGui::TextColored(tokenColor(reader), ";" + reader.getTokComment());
            break;
            
            // Linebreak is implicit in DearIMGUI, no action needed
            case TOKEN_TYPE_LINEBREAK:
            if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
            {
                ImGui::SameLine();
            }
            ImGui::TextColored(tokenColor(reader), "<br>");
            //  ImGui::SameLine();  ImGui::Text(""); // hack to fix highlight of last token on line.
            break;
            
            // Other tokens come anywhere - delimiting logic is needed
            default:
            if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
            {
                ImGui::SameLine();
                //    string delimiter = (reader.tokenType(-1) == TOKEN_TYPE_KEYWORD) ? " " : ", ";
                //    ImGui::Text(delimiter);
                //    ImGui::SameLine();
            }
            
            switch (reader.tokenType())
            {
                case TOKEN_TYPE_STRING:
                ImGui::TextColored(tokenColor(reader), "\"" + reader.getTokString() + "\"");
                break;
                case TOKEN_TYPE_FLOAT:
                {
                    ImGui::Text("" + reader.getTokFloat());
                    break;
                }
                case TOKEN_TYPE_INT:
                {
                    ImGui::Text("" + reader.getTokInt());
                    break;
                }
                case TOKEN_TYPE_BOOL:
                ImGui::TextColored(tokenColor(reader), ""+reader.getTokBool());
                break;
            }
        }
        
        if (ImGui::IsItemHovered()) { 
            hoveredTokenPos = reader.getPos() ;
            hover_found = true;
        }
        
        if (ImGui::IsItemClicked(0))
        {
            focusedTokenPos = reader.getPos(); 
        }
        
        reader.moveNext();
        
    }
    
    if (!hover_found) 
    {
        hoveredTokenPos = -1;
    }
    
    ImGui::PopID(); // "docBody"
}

color tokenColor(GenericDocContextClass@ reader)
{
    if (focusedTokenPos > -1 && reader.getPos() == uint(focusedTokenPos))
    {
        return color(0.9f, 0.1f, 0.1f, 1.f);
    }
    
    if (hoveredTokenPos > -1 && reader.getPos() == uint(hoveredTokenPos))
    {
        return color(0.1f, 1.f, 0.1f, 1.f);
    }
    
    switch (reader.tokenType())
    {
        case TOKEN_TYPE_KEYWORD:            return color(1.f, 1.f, 0.f, 1.f);
        case TOKEN_TYPE_COMMENT:               return color(0.5f, 0.5f, 0.5f, 1.f);
        case TOKEN_TYPE_STRING:               return color(0.f, 1.f, 1.f, 1.f);
        case TOKEN_TYPE_FLOAT:                return color(0.9, 0.8, 0.9, 1.f);
        case TOKEN_TYPE_INT:                return color(0.9, 0.9, 0.8, 1.f);
        case TOKEN_TYPE_BOOL:              return color(1.f, 0.f, 1.f, 1.f);      
        case TOKEN_TYPE_LINEBREAK:      return color(0.66f, 0.55f, 0.33f, 1.f);
    } // end switch
    return color(0.9, 0.9, 0.9, 1.f);
} 

void drawTokenEditPanel()
{
    if ( focusedTokenPos == -1)
    {
        ImGui::TextDisabled("Use left mouse button to select tokens.");
        return; // nothing to draw
    }
    GenericDocContextClass reader(g_displayed_document);
    while (!reader.endOfFile() && reader.getPos() != uint(focusedTokenPos)) reader.moveNext();
    if (reader.endOfFile())
    {
        ImGui::Text("EOF!!");
    }
    else
    {
        ImGui::TextDisabled("Token pos: "); ImGui::SameLine(); ImGui::Text("" + reader.getPos());
        ImGui::TextDisabled("Token type: "); ImGui::SameLine(); ImGui::Text(tokenTypeStr(reader.tokenType()));
    }
}

string tokenTypeStr(TokenType t)
{
    switch (t)
    {
        case TOKEN_TYPE_FLOAT: return "Float";
        case TOKEN_TYPE_INT: return "Integer";
        case TOKEN_TYPE_STRING: return "String";
        case TOKEN_TYPE_BOOL:  return "Boolean";
        case TOKEN_TYPE_COMMENT: return "Comment";
        case TOKEN_TYPE_LINEBREAK: return "Line break";
        case TOKEN_TYPE_KEYWORD: return "Keyword";
    }
    return "?";
}

//#endregion


