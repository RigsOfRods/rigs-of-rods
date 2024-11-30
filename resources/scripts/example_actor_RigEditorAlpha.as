/// \file Prototype truck editor, Oct 2023
/// \brief Showcases "gdeditor_utils.as" and "gridviewer_utils.as"
///
/// Lets you import ZIPped mod as project (directory under /projects).
/// Unfinished - no actual editing (to be worked into 'gdeditor_utils.as')
/// see https://github.com/RigsOfRods/rigs-of-rods/pull/3048
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler; 

// node/beam drawing
#include "gridviewer_utils.as"
gridviewer_utils::GridViewer viewer_x;
gridviewer_utils::GridViewer viewer_y;
gridviewer_utils:: GridViewer viewer_z; 

// genericDoc editor
#include "genericdoc_utils.as"
genericdoc_utils::GenericDocEditor gdEditor;

// ----- config -----

int m_statusbar_height_pixels = 25;

// ---- variables -----

GenericDocumentClass@ m_displayed_document = null;

string m_error_str;
string m_project_creation_pending; // name of the truck file which will become available in modcache.
CacheEntryClass@ m_project_entry;

dictionary@ m_modcache_results = null;
CacheEntryClass@ m_awaiting_load_bundle_entry = null;
color node_color = color(0.8, 0.9, 0.2, 1.0);
float node_radius = 1.f;    


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
        if (@entry != null && entry.resource_bundle_type == "FileSystem")
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
                    
                    gdEditor.drawDocumentBody();
                }
            }
            ImGui::EndChild(); // "docBody"
            
            
            ImGui::Separator();
            gdEditor.drawTokenEditPanel();
            
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
    gdEditor.setDocument(m_displayed_document);
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
    
    gdEditor.setDocument(m_displayed_document);
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

