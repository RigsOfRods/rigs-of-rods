/// \file Prototype truck editor, Oct 2023
/// \brief Showcases truck file editing features
/// see https://github.com/RigsOfRods/rigs-of-rods/pull/3048
/// Written and auto-indented using script_editor.as!
// ===================================================

#include "imgui_utils.as"
#include "gridviewer_utils.as"

// ----- config -----

int m_statusbar_height_pixels = 25;

// ---- variables -----

GenericDocumentClass@ m_displayed_document = null;
int m_hovered_token_pos = -1;
int m_focused_token_pos = -1;
string m_error_str;
string m_project_creation_pending; // name of the truck file which will become available in modcache.
CacheEntryClass@ m_project_entry;
gridviewer_utils::GridViewer viewer_x;
gridviewer_utils::GridViewer viewer_y;
gridviewer_utils:: GridViewer viewer_z; 
dictionary@ m_modcache_results = null;
CacheEntryClass@ m_awaiting_load_bundle_entry = null;
color node_color = color(0.8, 0.9, 0.2, 1.0);
float node_radius = 1.f;    
imgui_utils::CloseWindowPrompt closeBtnHandler; // Window [X] button handler

// ---- functions ----

// STARTUP
void main()
{
    viewer_x.childWindowID = "demoX"; 
    viewer_x.hAxis=1; 
    viewer_x.vAxis=2; // axes: YZ
    
    viewer_y.childWindowID = "demoY"; 
    viewer_y.hAxis=0; 
    viewer_y.vAxis=2; // axes: XZ
    
    viewer_z.childWindowID = "demoZ";  // axes  XY (default)
    
}

// RENDERING
void frameStep(float dt)
{
    // check if the actor is already a project
    BeamClass@ actor = game.getCurrentTruck();
    if (@actor != null)
    {
        CacheEntryClass@ entry =  modcache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, actor.getTruckFileName());
        if (@entry != null)
        {
            @m_project_entry = @entry;
            loadDocument();
        }
    }
    
    checkForCreatedProject();
    drawWindow();
}

// NOTIFICATIONS
void eventCallbackEx(scriptEvents ev, int a1, int a2, int a3, int a4, string a5, string a6, string a7, string a8)
{
    if (ev == SE_GENERIC_MODCACHE_ACTIVITY && a1 == MODCACHEACTIVITY_BUNDLE_LOADED)
    {
        onEventBundleLoaded(a2);
    }
}


//#region Draw surrounding window

void drawWindow()
{
    
    // Begin window
    
    string caption = "RigEditor";
    if (@m_project_entry != null)
    {
        caption += " ("+m_project_entry.fname+")";
    }
    int flags = ImGuiWindowFlags_MenuBar;
    ImGui::Begin(caption, closeBtnHandler.windowOpen, flags);
    closeBtnHandler.draw();
    
    // Draw menu bar
    
    drawMenubar();
    
    // Split off top-pane (all editing windows) from bottom statusbar
    vector2 fullsize_with_statusbar = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
    vector2 fullsize = fullsize_with_statusbar - vector2(0, m_statusbar_height_pixels);
    if (ImGui::BeginChild("mainPane", fullsize))
    {
        
        // Draw the document pane (left side)
        
        
        vector2 leftpane_size = fullsize * vector2(0.3, 1.f);
        
        if (ImGui::BeginChild("leftPane", leftpane_size))
        {
            
            if (ImGui::BeginChild("docBody", leftpane_size * vector2(1.f, 0.8)))
            {
                if (@m_displayed_document != null)
                {            
                    drawDocumentBody();
                }
            }
            ImGui::EndChild(); // "docBody"
            
            if ( m_focused_token_pos > -1)
            {
                ImGui::Separator();
                drawTokenEditPanel();
            }      
        }
        ImGui::EndChild(); // "leftPane"
        ImGui::SameLine();
        // Draw the right side
        vector2 rightpane_size = fullsize - vector2(leftpane_size.x, 0);
        if (ImGui::BeginChild("rightPane", rightpane_size))
        {
            
            vector2 views_size = fullsize * vector2(0.7, 1.f);
            vector2 qsize = views_size*vector2(0.5, 0.5);
            
            viewer_x.begin(qsize);
            drawNodes(viewer_x);
            viewer_x.end();
            
            ImGui::SameLine();
            
            viewer_y.begin(qsize);
            drawNodes(viewer_y);
            viewer_y.end();
            
            viewer_z.begin(qsize);
            drawNodes(viewer_z);
            viewer_z.end();
        }
        ImGui::EndChild(); // "rightPane"
    }
    ImGui::EndChild(); // "mainPane"
    
    // draw statusbar
    
    drawStatusbar(); 
    
    // End window
    
    ImGui::End();
    
}

void drawMenubar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Open project"))
        {              
            if (@m_modcache_results == null || ImGui::Button("Refresh"))
            {
                @m_modcache_results = modcache.query({
                {'filter_type', LOADER_TYPE_ALLBEAM},
                {'filter_category_id', 8990} // CID_Projects
                });
            }
            
            if (@m_modcache_results != null)
            {
                array<CacheEntryClass@> result_entries = cast<array<CacheEntryClass@>>(m_modcache_results['entries']);
                for (uint i = 0; i < result_entries.length(); i++)
                {
                    ImGui::PushID(i);
                    
                    ImGui::Bullet();
                    ImGui::SameLine();
                    ImGui::Text(result_entries[i].dname);
                    ImGui::SameLine();
                    ImGui::TextDisabled("("+result_entries[i].fname+")");
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Load"))
                    {
                game.pushMessage(MSG_EDI_LOAD_BUNDLE_REQUESTED, { {'cache_entry', result_entries[i]} });
                        // we will receive `MODCACHEACTIVITY_BUNDLE_LOADED` - see `eventCallbackEx()` at the end of file.
                        @m_awaiting_load_bundle_entry = result_entries[i];
                        game.registerForEvent(SE_GENERIC_MODCACHE_ACTIVITY);
                    }
                    
                    ImGui::PopID(); // i
                }
            }
            else
            {
                ImGui::Text("ModCache query FAILED!");
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Manage projects"))
        {
            ImGui::TextDisabled("Create a project:");
            
            BeamClass@ actor = game.getCurrentTruck();
            if (@actor != null)
            {
                // Actor name and "View document" button
                ImGui::PushID("actor");
                ImGui::AlignTextToFramePadding();
                ImGui::Text("You are driving " + actor.getTruckName());
                ImGui::SameLine();
                if (@m_displayed_document == null)
                {
                    if (ImGui::SmallButton("Import as project"))
                    {
                        createProjectFromActorAsync(actor);
                    }
                }
                
                ImGui::PopID(); //"actor"
            }
            else
            {
                ImGui::Text("You are on foot. Spawn a vehicle to access it's definition file.");
            }            
            
            ImGui::EndMenu();
        }
        
        
        ImGui::EndMenuBar();
    }
}

void drawStatusbar()
{
    if (m_error_str != "")
    {
        ImGui::Text("ERROR! " + m_error_str);
        if (ImGui::SmallButton("Clear"))
        {
            m_error_str = "";
        }
        return;
    }
    else if (@m_displayed_document != null)
    {
        ImGui::TextDisabled("Click tokens with mouse to edit. Spawn as usual (use category 'Projects')");
        ImGui::SameLine();
        if (ImGui::SmallButton("Save file"))
        {
            m_displayed_document.saveToResource(m_project_entry.fname, m_project_entry.resource_group);
        }
    }
    else if (m_project_creation_pending != "") 
    {
        ImGui::Text ("Waiting for project to be created...");
    }        
    else if (@m_awaiting_load_bundle_entry != null) 
    {
        ImGui::Text ("Loading bundle '"+m_awaiting_load_bundle_entry.dname+"'...");
    }
    else
    {
        ImGui::Text("Ready to load or import project");
    }
}
//#endregion

// #region Project management

void createProjectFromActorAsync(BeamClass@ actor )
{
    // Fetch the current actor cache entry
    CacheEntryClass@ src_entry = modcache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, actor.getTruckFileName());
    if (@src_entry == null)
    m_error_str = "Failed to load cache entry!";
    
    // request project to be created from that cache entry
    string proj_name = "project1";
    game.pushMessage(MSG_EDI_CREATE_PROJECT_REQUESTED, {
    {'name', proj_name},
    {'source_entry', src_entry}
    });
    
    // Now we have to wait until next frame for the project to be created
    m_project_creation_pending = proj_name + "." + src_entry.fext; // there's no notification event yet, we must poll.
}

void loadDocument()
{
    GenericDocumentClass@ doc = GenericDocumentClass();
    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE
    | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
    | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;
    if (!doc.loadFromResource(m_project_entry.fname, m_project_entry.resource_group, flags))
    {
        m_error_str = "Project file failed to load!";
        return;
    }
    @m_displayed_document = doc;
}

void loadAndFixupDocument()
{
    loadDocument();
    GenericDocumentClass@ doc = GenericDocumentClass();
    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE
    | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
    | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;
    if (!doc.loadFromResource(m_project_entry.fname, m_project_entry.resource_group, flags))
    {
        m_error_str = "Project file failed to load!";
        return;
    }
    @m_displayed_document = doc;
    
    // fixup the document.. 
    
    GenericDocContextClass@ ctx = GenericDocContextClass(m_displayed_document);
    // >> seek the name
    while (!ctx.endOfFile() && !ctx.isTokString()) {
        ctx.seekNextLine(); 
    }
    // >> change the name
    if (!ctx.endOfFile()) {
        ctx.setTokString(0, m_project_entry.dname);
    }
    // >> seek fileinfo 
    while (!ctx.endOfFile() && (!ctx.isTokKeyword() || ctx.getTokKeyword() != 'fileinfo')) {
        ctx.seekNextLine(); 
    }
    // change the fileinfo param #2 categoryid
    if (!ctx.endOfFile(2)) {
        ctx.setTokFloat(2, 8990); // special "Project" category
    }
}

// #endregion

// #region Tokenized document drawing

void drawDocumentBody()
{    
    ImGui::PushID("docBody");
    bool hover_found = false;
    
    GenericDocContextClass reader(m_displayed_document);
    while (!reader.endOfFile())
    {
        
        switch (reader.tokenType())
        {
            // These tokens are always at start of line
            case TOKEN_TYPE_KEYWORD: {
                ImGui::TextColored(tokenColor(reader), reader.getTokKeyword());
                break;
            }
            case TOKEN_TYPE_COMMENT: {
                ImGui::TextColored(tokenColor(reader), ";" + reader.getTokComment());
                break;
            }
            
            // Linebreak is implicit in DearIMGUI, no action needed
            case TOKEN_TYPE_LINEBREAK: {
                if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
                {
                    ImGui::SameLine();
                }
                ImGui::TextColored(tokenColor(reader), "<br>");
                //  ImGui::SameLine();  ImGui::Text(""); // hack to fix highlight of last token on line.
                break;
            }
            
            // Other tokens come anywhere - delimiting logic is needed
            default: {
                if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
                {
                    ImGui::SameLine();
                    //    string delimiter = (reader.tokenType(-1) == TOKEN_TYPE_KEYWORD) ? " " : ", ";
                    //    ImGui::Text(delimiter);
                    //    ImGui::SameLine();
                }
            }
            
            switch (reader.tokenType())
            {
                case TOKEN_TYPE_STRING: {
                    ImGui::TextColored(tokenColor(reader), "\"" + reader.getTokString() + "\"");
                    break;
                }
                case TOKEN_TYPE_NUMBER: {
                    ImGui::TextColored(tokenColor(reader), "" + reader.getTokFloat());
                    break;
                }
                case TOKEN_TYPE_BOOL: {
                    ImGui::TextColored(tokenColor(reader), ""+reader.getTokBool());
                    break;
                }
            }
        }
        
        if (ImGui::IsItemHovered()) { 
            m_hovered_token_pos = reader.getPos() ;
            hover_found = true;
        }
        
        if (ImGui::IsItemClicked(0))
        {
            m_focused_token_pos = reader.getPos(); 
        }
        
        reader.moveNext();
        
    }
    
    if (!hover_found) 
    {
        m_hovered_token_pos = -1;
    }
    
    ImGui::PopID(); // "docBody"
}

void drawTokenEditPanel()
{
    GenericDocContextClass reader(m_displayed_document);
    while (!reader.endOfFile() && (reader.getPos() != uint(m_focused_token_pos)))
    {
        reader.moveNext();
    }
    
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

// #endregion

// #region GridViewer node drawing

void drawNodes(gridviewer_utils::GridViewer @viewer)
{
    // This must run in between `GridViewer.begin()` and `GridViewer.end()` !
    // ----------------------------------------------------------------------
    
    if (@m_displayed_document == null)
    {
        return;
    }
    
    // Do it the slow way, reading directly from the GenericDocumentClass without any sort of caching.
    GenericDocContextClass ctx(m_displayed_document);
    
    // Seek to 'nodes' (assume there's just 1 'nodes' segment in the file - usual case)
    while (!ctx.endOfFile() && (!ctx.isTokKeyword() || ctx.getTokKeyword() != "nodes"))
    {
        ctx.seekNextLine();
    }
    ctx.seekNextLine();
    while (!ctx.endOfFile())
    {
        //int node_num = int(ctx.getTokFloat(0));
        vector3 node_pos;
        if (ctx.isTokFloat(1) && ctx.isTokFloat(2) && ctx.isTokFloat(3))
        {
            node_pos.x = ctx.getTokFloat(1);
            node_pos.y = ctx.getTokFloat(2);
            node_pos.z = ctx.getTokFloat(3);
            ImGui::GetWindowDrawList().AddCircleFilled( viewer.localToScreenPos(node_pos), node_radius, node_color);
        }
        
        ctx.seekNextLine();
        
        
        if (ctx.isTokKeyword() && ctx.getTokKeyword() != "set_node_defaults")
        {
            break;
        }
    }
}   

// #endregion

// #region Token drawing helpers
string tokenTypeStr(TokenType t)
{
    switch (t)
    {
        case TOKEN_TYPE_NUMBER: return "Number";
        case TOKEN_TYPE_STRING: return "String";
        case TOKEN_TYPE_BOOL:  return "Boolean";
        case TOKEN_TYPE_COMMENT: return "Comment";
        case TOKEN_TYPE_LINEBREAK: return "Line break";
        case TOKEN_TYPE_KEYWORD: return "Keyword";
    }
    return "?";
} 

color tokenColor(GenericDocContextClass@ reader)
{
    if (m_focused_token_pos > -1 && reader.getPos() == uint(m_focused_token_pos))
    {
        return color(0.9f, 0.1f, 0.1f, 1.f);
    }
    
    if (m_hovered_token_pos > -1 && reader.getPos() == uint(m_hovered_token_pos))
    {
        return color(0.1f, 1.f, 0.1f, 1.f);
    }
    
    switch (reader.tokenType())
    {
        case TOKEN_TYPE_KEYWORD:        return color(1.f, 1.f, 0.f, 1.f);
        case TOKEN_TYPE_COMMENT:        return color(0.5f, 0.5f, 0.5f, 1.f);
        case TOKEN_TYPE_STRING:         return color(0.f, 1.f, 1.f, 1.f);
        case TOKEN_TYPE_NUMBER:         return color(0.9, 0.9, 0.9, 1.f);
        case TOKEN_TYPE_BOOL:           return color(1.f, 0.f, 1.f, 1.f);      
        case TOKEN_TYPE_LINEBREAK:      return color(0.66f, 0.55f, 0.33f, 1.f);
    }
    return color(0.9, 0.9, 0.9, 1.f);
}
// #endregion

// #region Event handling or polling
void checkForCreatedProject()
{
    // there's no notification event for completed request, we must poll like this.
    if (m_project_creation_pending != "")
    {
        @m_project_entry =  modcache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, m_project_creation_pending);
        if (@m_project_entry != null)
        {
            // success!! stop polling and open the document.
            m_project_creation_pending="";
            loadAndFixupDocument();
        }
    }
}

void onEventBundleLoaded(int cache_number)
{
    //game.log("DBG onEventBundleLoaded(): number "+cache_number);
    if (cache_number == m_awaiting_load_bundle_entry.number)
    {
        @m_project_entry =  @m_awaiting_load_bundle_entry;
        @m_awaiting_load_bundle_entry= null;
        
        game.unRegisterEvent(SE_GENERIC_MODCACHE_ACTIVITY);
        loadAndFixupDocument();
    }
}
// #endregion

