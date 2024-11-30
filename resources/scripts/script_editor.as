// Project Rigs of Rods (http://www.rigsofrods.org)
//              ** SCRIPT EDITOR **
// Documentation: http://developer.rigsofrods.org
// ================================================

#include "imgui_utils.as" // ImHyperlink()
#include "scriptinfo_utils.as" // class ScriptInfo

// Global definitions:
// -------------------

// from <angelscript.h>
enum asEMsgType
{
    asMSGTYPE_ERROR       = 0,
    asMSGTYPE_WARNING     = 1,
    asMSGTYPE_INFORMATION = 2
};
// from rigs of rods
int SCRIPTUNITID_INVALID = -1;
const string RGN_SCRIPTS = "Scripts";
const string RGN_RESOURCES_SCRIPTS = "ScriptsRG";
// others
const string BUFFER_TEXTINPUT_LABEL = "##bufferTextInputLbl";
const string TABBAR_ID = "##scriptEditorTabs";
const int BUFFER_MIN_SIZE = 10000;
const int BUFFER_INCREMENT_SIZE = 2500;
const color LINKCOLOR = color(0.3, 0.5, 0.9, 1.0);
ScriptEditorWindow editorWindow;
const uint FILEINFO_COMPRESSEDSIZE_UNKNOWN = uint(-1);

// Config:
// -------

// LINE METAINFO COLUMNS (drawn by `drawLineInfoColumns()`, space calculated by `measureLineInfoColumns()`, visibility toggled by `drawMenubar()`)
// NOTE: most of these are early debug helpers, I kept them around (some visible by default) for the laughs.
// Line numbers
bool drawLineNumbers=true;
color lineNumberColor=color(1.f,1.f,0.7f,1.f);
float lineNumColumnWidth = 25;
// Line char counts
bool drawLineLengths=true;
color lineLenColor=color(0.5f,0.4f,0.3f,1.f);
float lineLenColumnWidth = 25;
// Line comment offsets
bool drawLineCommentOffsets=true;
color lineCommentOffsetColor=color(0.1f,0.9f,0.6f,0.6f);
float lineCommentColumnWidth = 25;
// Line http offsets
bool drawLineHttpOffsets=false; // only visual, links don't open
color lineHttpOffsetColor=color(LINKCOLOR.r, LINKCOLOR.g, LINKCOLOR.a, 0.6f);
float lineHttpColumnWidth = 16;
// Line err counts (error may span multiple AngelScript messages!)
bool drawLineErrCounts = false;
color lineErrCountColor = color(1.f,0.2f,0.1f,1.f);
float errCountColumnWidth = 10;
// Auto-indentation level display
bool drawLineAutoIndentLevels = true;
color lineAutoIndentLevelsColor = color(0.55f, 0.44f, 0.55f, 1.f);
float lineAutoIndentLevelsColumnWidth = 16;
// Actual indent display
bool drawLineActualIndents = true;
color lineActualIndentsColor = color(0.55f, 0.55f, 0.44f, 1.f);
float lineActualIndentsColumnWidth = 16;
// Line region&endregion markers
bool drawLineRegionMarkers = true;
color lineRegionMarkerColor = color(0.78f,0.76f,0.32f,1.f);
float lineRegionMarkerColumnWidth = 17;
// END line metainfo columns

// EDITOR settings
color errorTextColor = color(1.f,0.3f,0.2f,1.f);
color errorTextBgColor = color(0.1f,0.04f,0.08f,1.f);
bool drawErrorText = true;
color warnTextColor = color(0.78f,0.66f,0.22f,1.f);
color warnTextBgColor = color(0.1f,0.08f,0.0f,1.f);
bool drawWarnText = true;
color commentHighlightColor = color(0.1f,0.9f,0.6f,0.16f);
color regionCommentHighlightColor = color(0.78f,0.76f,0.32f,0.18f);
bool drawCommentHighlights = true;
color httpHighlightColor=color(0.25, 0.3, 1, 0.3f);
bool drawHttpHighlights = false; // only visual, links don't open
float scriptInfoIndentWidth = 25;

int autoIndentNumSpaces = 4;
bool autoIndentOnSave = true;

bool killScriptOnAngelscriptException = true; // Prevent endless assert() loop "mismatched BeginChild()/EndChild()" under Visual Studio Debug, effectivelly forcing you to stop the game.

// input output
bool saveShouldOverwrite=false;

float autoSaveIntervalSec=25.f;
color autoSaveStatusbarBgColor = color(0.3f, 0.3f, 0.4f, 1.f);
bool drawAutosaveCountdown=false;
// END editor

// Callback functions for the game:
// --------------------------------

void main() // Invoked by the game on startup
{
    game.log("Script editor started!");
    game.registerForEvent(SE_ANGELSCRIPT_MSGCALLBACK);
    game.registerForEvent(SE_ANGELSCRIPT_EXCEPTIONCALLBACK);
    game.registerForEvent(SE_ANGELSCRIPT_MANIPULATIONS);
    game.registerForEvent(SE_GENERIC_EXCEPTION_CAUGHT);
    @editorWindow.recentScriptsRecord.fileinfos = array<dictionary>(); // special - constructed manually
}

void frameStep(float dt) // Invoked regularly by the game; the parameter is elapsed time in seconds.
{
    editorWindow.draw(dt);
}


void eventCallbackEx(scriptEvents ev, // Invoked by the game when a registered event is triggered
int arg1, int arg2ex, int arg3ex, int arg4ex,
string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    if (ev == SE_ANGELSCRIPT_MSGCALLBACK)
    {
        for (uint i=0; i<editorWindow.tabs.length(); i++)
        {
            editorWindow.tabs[i].onEventAngelScriptMsg(
            /*id:*/arg1, /*eType:*/arg2ex, /*row:*/arg3ex, /*col:*/arg4ex, // ints
            /*sectionName:*/arg5ex, /*msg:*/arg6ex); // strings
        }
    }
    else if (ev == SE_ANGELSCRIPT_MANIPULATIONS)
    {
        for (uint i=0; i<editorWindow.tabs.length(); i++)
        {
            editorWindow.tabs[i].onEventAngelScriptManip(
            /*manipType:*/arg1, /*nid:*/arg2ex, /*category:*/arg3ex, // ints
            /*name:*/arg5ex); // strings
        }
    }
    else if (ev == SE_GENERIC_EXCEPTION_CAUGHT)
    {
        for (uint i=0; i<editorWindow.tabs.length(); i++)
        {
            editorWindow.tabs[i].epanel.onEventExceptionCaught(arg1, arg5ex, arg6ex, arg7ex);
        }
    }
    else if (ev == SE_ANGELSCRIPT_EXCEPTIONCALLBACK)
    {
        int nid = arg1;
        /*game.log("DBG eventCallbackEx(EXCEPTIONCALLBACK): distributing to "+ editorWindow.tabs.length()+ " tabs, NID: " + nid);*/

        for (uint i=0; i<editorWindow.tabs.length(); i++)
        {
            editorWindow.tabs[i].epanel.onEventAngelscriptExceptionCallback(
            nid, /*arg3_linenum:*/arg3ex, /*arg5_from:*/arg5ex, /*arg6_msg:*/arg6ex);
        }

        if (killScriptOnAngelscriptException)
        {
            dictionary@ scriptInfo = game.getScriptDetails(nid);
            game.log("ScriptEditor: Killing script '"+string(scriptInfo['scriptName'])+"' (NID "+nid+") because it triggered an AngelScript exception; you can disable this behavior in top menu 'Tools/Settings'.");
    game.pushMessage(MSG_APP_UNLOAD_SCRIPT_REQUESTED, { {'id', nid} });

            for (uint i=0; i<editorWindow.tabs.length(); i++)
            {
                if (editorWindow.tabs[i].currentScriptUnitID == nid)
                {
                    editorWindow.tabs[i].epanel.onEventAngelscriptExceptionCallback(
                    nid, /*arg3_linenum:*/0, /*arg5_from:*/"ScriptEditor",
                    /*arg6_msg:*/"Killing script because it triggered an AngelScript exception; you can disable this behavior in top menu 'Tools/Settings'.");

                    editorWindow.tabs[i].currentScriptUnitID = SCRIPTUNITID_INVALID; // Reset the RUN/STOP button, leave everything else as-is.
                }
            }
        }
    }
}

// The script editor logic:
// ------------------------

class ScriptEditorWindow
{
    // 'LOAD FILE' MENU:
    string fileNameBuf;
    // 'SAVE FILE' MENU:
    string saveFileNameBuf;
    bool saveMenuOpening = true;
    int saveFileResult = 0; // 0=none, -1=error, 1=success
    // 'EXAMPLES' MENU:
    string exampleNameBuf;
    // 'INCLUDES' MENU:
    string includeNameBuf;
    // TABS:
    array<ScriptEditorTab@> tabs;
    uint currentTab = 0;
    int tabScheduledForRemoval = -1;

    // GLOBAL CONTEXT
    imgui_utils::CloseWindowPrompt closeBtnHandler;

    // MENU CONTEXT (pre-scanned file lists with extra details)
    ScriptIndexerRecord recentScriptsRecord;
    ScriptIndexerRecord localScriptsRecord;
    ScriptIndexerRecord exampleScriptsRecord;
    ScriptIndexerRecord includeScriptsRecord;

    ScriptEditorWindow()
    {
        closeBtnHandler.cfgPromptText = "Really close script editor? You will lose any unsaved work.";
    }

    void refreshLocalFileList()
    {
        //NOTE: `recentScriptsRecord` is constructed manually.
        this.localScriptsRecord.scanResourceGroup(RGN_SCRIPTS, "*.as*");
        this.exampleScriptsRecord.scanResourceGroup(RGN_RESOURCES_SCRIPTS, "example_*.*");
        this.includeScriptsRecord.scanResourceGroup(RGN_RESOURCES_SCRIPTS, "*_utils.*");
    }

    void addTab(const string&in tabName, const string&in buffer)
    {
        ScriptEditorTab nTab;
        nTab.bufferName = tabName;
        nTab.setBuffer(buffer);
        tabs.insertLast(nTab);
    }

    void removeTab(uint i)
    {
        if (i < tabs.length())
        {
            tabs.removeAt(i);
        }
    }

    void draw(float dt)
    {
        // Process scheduled tab closing
        if (this.tabScheduledForRemoval != -1)
        {
            this.removeTab(uint(tabScheduledForRemoval));
            this.tabScheduledForRemoval = -1;
        }

        // Make sure there's always a tab open ()
        if (tabs.length() == 0)
        {
            this.addTab("Tutorial.as", TUT_SCRIPT);
            this.currentTab = 0;
        }
        else
        {
            if (this.currentTab >= tabs.length())
            this.currentTab = tabs.length()-1;
        }

        this.tabs[this.currentTab].updateAutosave(dt);
        this.tabs[this.currentTab].handleRequests(dt);

        int flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar;

        if (ImGui::Begin("Script editor", this.closeBtnHandler.windowOpen, flags))
        {
            this.closeBtnHandler.draw();
            this.drawMenubar();
            this.drawTabBar();

            this.tabs[this.currentTab].drawTextArea();
            this.tabs[this.currentTab].drawExceptionsPanel();
            this.tabs[this.currentTab].drawFooter();

            // To draw on top of editor text, we must trick DearIMGUI using an extra invisible window
            ImGui::SetCursorPos(this.tabs[this.currentTab].errorsScreenCursor - ImGui::GetWindowPos());
            ImGui::PushStyleColor(ImGuiCol_ChildBg, color(0.f,0.f,0.f,0.f)); // Fully transparent background!
            int childFlags = ImGuiWindowFlags_NoInputs; // Necessary, otherwise editing is blocked.
            if (ImGui::BeginChild("ScriptEditorErr-child", this.tabs[this.currentTab].errorsTextAreaSize,/*border:*/false, childFlags))
            {
                this.tabs[this.currentTab].drawTextAreaErrors();
            }
            ImGui::EndChild(); // must always be called - legacy reasons
            ImGui::PopStyleColor(1); // ChildBg
        }
        ImGui::End();

    }

    private void drawTabBar()
    {
        int tabBarFlags = 0;
        if (ImGui::BeginTabBar(TABBAR_ID, tabBarFlags))
        {
            for (uint i = 0; i < this.tabs.length(); i++)
            {
                bool tabOpen = true;
                int tabFlags = 0;
                if (this.currentTab == i)
                {
                    tabFlags = ImGuiTabItemFlags_SetSelected;
                }
                bool tabDrawn = ImGui::BeginTabItem(this.tabs[i].bufferName, /*inout:*/tabOpen, tabFlags);
                if (tabDrawn)
                {
                    ImGui::EndTabItem();
                }
                if (!tabOpen)
                this.tabScheduledForRemoval = int(i);
                else if (ImGui::IsItemClicked())
                this.currentTab = i;
            }

            ImGui::EndTabBar();
        }
    }

    private void drawMenubar()
    {
        bool analysisDoneThisFrame = false;
        if (ImGui::BeginMenuBar())
        {
            // 'OPEN FILE' menu
            if (ImGui::BeginMenu("Open file"))
            {
                if (!analysisDoneThisFrame)
                {
                    analysisDoneThisFrame = localScriptsRecord.advanceScriptAnalysis();
                }

                bool loadRecentFile = this.drawSelectableFileList("Recent scripts", "Load##recent", recentScriptsRecord, /*&inout*/ fileNameBuf);
                ImGui::Separator();
                bool loadLocalFile = this.drawSelectableFileList("Local scripts", "Load##local", localScriptsRecord, /*&inout*/ fileNameBuf);

                if (loadRecentFile || loadLocalFile)
                {
                    // Recognize auto-saves and load them under the original name. Note '_AUTOSAVE' ~ 9 characters.
                    string tabName = fileNameBuf;
                    string baseName = fileNameBuf; // filename without extension
                    int dotPos = fileNameBuf.findLast(".");
                    if (dotPos > 0)
                    {
                        baseName = fileNameBuf.substr(0, dotPos);
                    }
                    if (baseName.length() > 9 && baseName.substr(baseName.length()-9) == "_AUTOSAVE")
                    {
                        tabName = baseName.substr(0, baseName.length()-9);
                        if (dotPos > 0)
                        {
                            tabName += fileNameBuf.substr(dotPos);
                        }
                        /*game.log("DBG script editor: open file menu: file '"+fileNameBuf+"' is an autosave, loading as '"+tabName+"'");*/
                    }

                    this.addTab(tabName, game.loadTextResourceAsString(fileNameBuf, RGN_SCRIPTS));
                    this.currentTab = this.tabs.length() - 1; // Focus the new tab
                    this.addRecentScript(fileNameBuf);
                }

                ImGui::EndMenu();
            }

            // 'SAVE FILE' menu
            if (ImGui::BeginMenu("Save file"))
            {
                if (!analysisDoneThisFrame)
                {
                    analysisDoneThisFrame = localScriptsRecord.advanceScriptAnalysis();
                }

                if (this.saveMenuOpening)
                {
                    saveFileNameBuf = this.tabs[this.currentTab].bufferName;
                    this.saveMenuOpening = false;
                    this.saveFileResult = 0;
                }
                ImGui::InputText("File",/*inout:*/ saveFileNameBuf);
                if (ImGui::Button("Save"))
                {
                    this.tabs[this.currentTab].requestSaveFile = true;
                }
                ImGui::SameLine();
                ImGui::Checkbox("Overwrite", /*inout:*/saveShouldOverwrite);

                // Error indicator:
                if (this.saveFileResult == 1)
                ImGui::TextColored(color(0.2,0.7, 0.2, 1), "File saved OK");
                else if (this.saveFileResult == -1)
                ImGui::TextColored(color(1,0.1, 0.2, 1), "Error saving file!");

                ImGui::Separator();
                this.drawSelectableFileList("Recent scripts", "Select##recent", recentScriptsRecord, /*&inout*/ saveFileNameBuf);
                this.drawSelectableFileList("Local scripts", "Select##local", localScriptsRecord, /*&inout*/ saveFileNameBuf);
                ImGui::EndMenu();
            }
            else
            {
                this.saveMenuOpening = true;
            }

            // 'EXAMPLES' menu
            if (ImGui::BeginMenu("Examples"))
            {
                if (!analysisDoneThisFrame)
                {
                    analysisDoneThisFrame = exampleScriptsRecord.advanceScriptAnalysis();
                }

                bool loadExampleFile = this.drawSelectableFileList("Example scripts", "Load##example", exampleScriptsRecord, /*&inout*/ exampleNameBuf);

                if (loadExampleFile)
                {
                    this.addTab(exampleNameBuf, game.loadTextResourceAsString(exampleNameBuf, RGN_RESOURCES_SCRIPTS));
                    this.currentTab = this.tabs.length() - 1; // Focus the new tab
                }

                ImGui::EndMenu();
            }

            // 'INCLUDES' menu
            if (ImGui::BeginMenu("Includes"))
            {
                if (!analysisDoneThisFrame)
                {
                    analysisDoneThisFrame = includeScriptsRecord.advanceScriptAnalysis();
                }

                bool loadIncludeFile = this.drawSelectableFileList("Include scripts", "Load##include", includeScriptsRecord, /*&inout*/ includeNameBuf);

                if (loadIncludeFile)
                {
                    this.addTab(includeNameBuf, game.loadTextResourceAsString(includeNameBuf, RGN_RESOURCES_SCRIPTS));
                    this.currentTab = this.tabs.length() - 1; // Focus the new tab
                }

                ImGui::EndMenu();
            }

            // 'VIEW' menu
            if (ImGui::BeginMenu("View"))
            {
                ImGui::TextDisabled("Line info:");
                ImGui::PushStyleColor(ImGuiCol_Text, lineNumberColor);
                ImGui::Checkbox("Line numbers", /*inout:*/drawLineNumbers);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineLenColor);
                ImGui::Checkbox("Char counts", /*inout:*/drawLineLengths);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineCommentOffsetColor);
                ImGui::Checkbox("Comment offsets", /*inout:*/drawLineCommentOffsets);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineHttpOffsetColor);
                ImGui::Checkbox("Http offsets", /*inout:*/drawLineHttpOffsets);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineErrCountColor);
                ImGui::Checkbox("Error counts", /*inout:*/drawLineErrCounts);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineRegionMarkerColor);
                ImGui::Checkbox("Region markers", /*inout:*/drawLineRegionMarkers);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineAutoIndentLevelsColor);
                ImGui::Checkbox("Auto indents", /*inout:*/drawLineAutoIndentLevels);
                ImGui::PopStyleColor(1); // ImGuiCol_Text
                ImGui::PushStyleColor(ImGuiCol_Text, lineActualIndentsColor);
                ImGui::Checkbox("Actual indents", /*inout:*/drawLineActualIndents);
                ImGui::PopStyleColor(1); // ImGuiCol_Text

                ImGui::TextDisabled("Editor overlay:");
                ImGui::Checkbox("Error text", /*inout:*/drawErrorText);
                ImGui::Checkbox("Warning text", /*inout:*/drawWarnText);
                ImGui::Checkbox("Comments", /*inout:*/drawCommentHighlights);
                ImGui::Checkbox("Hyperlinks", /*inout:*/drawHttpHighlights);

                ImGui::TextDisabled("Misc:");
                ImGui::Checkbox("Autosave countdown", /*inout:*/drawAutosaveCountdown);
                ImGui::EndMenu();
            }

            // 'FOLDING' menu
            if (ImGui::BeginMenu("Folding"))
            {
            if (ImGui::Button("Fold all")) { this.tabs[this.currentTab].requestFoldAll=true; }
            if (ImGui::Button("UnFold all")) { this.tabs[this.currentTab].requestUnFoldAll=true; }

                ImGui::Separator();
                ImGui::TextDisabled("Invalid states:");
                ImGui::TextColored(lineRegionMarkerColor, "  Un! = Unnamed");
                ImGui::TextColored(lineRegionMarkerColor, "  Br! = Broken (no end)");
                ImGui::TextColored(lineRegionMarkerColor, "  Ex! = Exists already");
                ImGui::TextColored(lineRegionMarkerColor, "  Or! = Orphan exists");

                ImGui::Separator();
                ImGui::TextDisabled("Orphan regions");
                array<string> regionNames = this.tabs[this.currentTab].workBufferRegions.getKeys();
                for (uint i = 0; i< regionNames.length(); i++)
                {
                    RegionInfo@ regionInfo = findRegion(this.tabs[this.currentTab].workBufferRegions, regionNames[i]);
                    if (@regionInfo != null && regionInfo.isOrphan)
                    {
                        ImGui::Bullet(); ImGui::SameLine(); ImGui::Text(regionNames[i]);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::BeginTooltip();
                            ImGui::Text(regionInfo.foldedOffText);
                            ImGui::EndTooltip();
                        }
                    }
                }
                ImGui::EndMenu();
            }

            // 'TOOLS' menu
            if (ImGui::BeginMenu("Tools"))
            {
                ImGui::TextDisabled("Tools:");
                if (ImGui::Button("Auto-indent"))
                {
                    this.tabs[this.currentTab].requestIndentBuffer = true;
                }

                ImGui::Separator();
                ImGui::TextDisabled("Settings");
                ImGui::SetNextItemWidth(75.f);
                ImGui::InputInt("Indent width", /*inout*/autoIndentNumSpaces);
                ImGui::Checkbox("Indent on save", /*inout*/autoIndentOnSave);
                ImGui::Dummy(vector2(5,5));
                ImGui::Checkbox("Kill script on AS exception", /*inout*/killScriptOnAngelscriptException);

                /*int nid = this.tabs[this.currentTab].currentScriptUnitID; // ~~~ EPanel debug util ~~~
                if (nid != SCRIPTUNITID_INVALID)
                {
                    ImGui::Separator();
                    ImGui::TextDisabled("DBG ExceptionPanel");
                if (ImGui::Button("Err1")) { this.tabs[this.currentTab].epanel.addExceptionInternal(nid, "Err1"); }
                if (ImGui::Button("ErrB")) { this.tabs[this.currentTab].epanel.addExceptionInternal(nid, "ErrB"); }
                if (ImGui::Button("Wtf")) { this.tabs[this.currentTab].epanel.addExceptionInternal(nid, "Wtf"); }
                if (ImGui::Button("Wat?")) { this.tabs[this.currentTab].epanel.addExceptionInternal(nid, "Wat?"); }
                }*/

                ImGui::EndMenu();
            }

            // 'DOCS' menu
            if (ImGui::BeginMenu("Docs"))
            {
                ImGui::TextDisabled("AngelScript:");
                imgui_utils::ImHyperlink('https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script.html', "The script language");

                ImGui::Separator();
                ImGui::TextDisabled("Rigs of Rods:");
                imgui_utils::ImHyperlink('https://developer.rigsofrods.org/d2/d42/group___script_side_a_p_is.html', "Script-side APIs");

                ImGui::EndMenu();
            }

            // START-STOP button
            ImGui::Dummy(vector2(50, 1));
            this.tabs[this.currentTab].drawStartStopButton();

            // AUTOSAVE counter
            if (drawAutosaveCountdown)
            {
                ImGui::Dummy(vector2(50, 1));
                this.tabs[this.currentTab].drawAutosaveStatusbar();
            }
            ImGui::EndMenuBar();
        }
    }

    // Creates a child window with dynamic height (but clamped to hardcoded max value).
    private bool drawSelectableFileList(string title, string btnText, ScriptIndexerRecord@ record, string&inout out_selection)
    {

        bool retval = false;
        ImGui::TextDisabled(title+" ("+record.fileinfos.length()+"):");
        if (record.fileinfos.length() == 0)
        {
            return retval; // nothing to draw
        }

        // Calc child window size
        const int MAX_CHILD_HEIGHT = 300;
        const int CHILD_WIDTH = 600;
        int childHeight = int(record.fileinfos.length() * ImGui::GetTextLineHeightWithSpacing());
    //ImGui::Text("DBG record.fileinfos.length()=" + record.fileinfos.length() + ", childHeight{not clamped}=" + childHeight);
        if (childHeight > MAX_CHILD_HEIGHT)
        {
            childHeight = MAX_CHILD_HEIGHT;
        }

        // draw the child window
        ImGui::BeginChild(title+"-child", vector2(CHILD_WIDTH, childHeight), /*border:*/false, ImGuiWindowFlags_HorizontalScrollbar);
        for (uint i=0; i<record.fileinfos.length(); i++)
        {
            ImGui::PushID(i);
            scriptinfo_utils::ScriptInfo@ scriptinfo = record.getScriptInfo(i);
            string filename = string(record.fileinfos[i]['filename']);
            uint size = uint(record.fileinfos[i]['compressedSize']);
            bool hovered=false;
            ImGui::Bullet();
            hovered = hovered || ImGui::IsItemHovered();
            ImGui::SameLine(); ImGui::Text(filename); hovered = hovered || ImGui::IsItemHovered();
            if(size != FILEINFO_COMPRESSEDSIZE_UNKNOWN)
            {
                ImGui::SameLine(); ImGui::TextDisabled("("+formatFloat(float(size)/1000.f, "", 3, 2)+" KB)"); hovered = hovered || ImGui::IsItemHovered();
            }
            ImGui::SameLine();
            if (hovered && (@scriptinfo != null))
            {
                ImGui::BeginTooltip();
                ImGui::TextColored(lineNumberColor, scriptinfo.title);
                ImGui::Text(scriptinfo.brief);
                ImGui::Separator();
                ImGui::TextWrapped(scriptinfo.text); ImGui::Dummy(vector2(300, 1));
                ImGui::EndTooltip();
            }
            if (ImGui::SmallButton(btnText))
            {
                out_selection = filename;
                retval = true;
            }
            ImGui::PopID(); // i
        }
        //   Always call a matching EndChild() for each BeginChild() call, regardless of its return value
        //[this is due to legacy reason and is inconsistent with most other functions
        ImGui::EndChild();

        return retval;
    }

    void addRecentScript(string fnameBuf)
    {
        if (fnameBuf == "")
        return; // nope

        for (uint i=0; i<recentScriptsRecord.fileinfos.length(); i++)
        {
            string filename = string(recentScriptsRecord.fileinfos[i]['filename']);
            if (filename == fnameBuf)
            return; // already exists
        }

        // if we got here, the name isn't in the list yet.
        //  Construct a pseudo-FileInfo format so we can use common helper.
        recentScriptsRecord.fileinfos.insertLast({
        {'filename', fnameBuf},
        {'compressedSize',FILEINFO_COMPRESSEDSIZE_UNKNOWN}
        } );
    }
}

class ScriptEditorTab
{
    // THE TEXT BUFFER
    string buffer; // The work buffer (doesn't contain folded regions!) - statically sized, determines maximum number of characters. DearIMGUI fills unused space by NULs.
    uint totalChars; // Actual number of characters before the NUL-ified space; Important for saving. // see `analyzeLines()`
    array<dictionary> bufferLinesMeta; // metadata for lines, see `analyzeLines()`
    array<array<uint>> bufferMessageIDs;
    dictionary workBufferRegions; // Key: region names (must be unique, otherwise not foldable), value: RegionInfo
    // END text buffer

    // MESSAGES FROM ANGELSCRIPT ENGINE
    array<dictionary> messages;
    int messagesTotalErr = 0;
    int messagesTotalWarn = 0;
    int messagesTotalInfo = 0;
    // END messages from angelscript


    // CONTEXT
    float scrollY = 0.f;
    float scrollX = 0.f;
    float scrollMaxY = 0.f;
    float scrollMaxX = 0.f;
    float errCountOffset=0.f;
    vector2 errorsScreenCursor(0,0);
    vector2 errorsTextAreaSize(0,0);
    vector2 textAreaSize(0,0);
    string bufferName = "EditorTab"; // Also tab name
    int currentScriptUnitID = SCRIPTUNITID_INVALID;
    bool waitingForManipEvent = false; // When waiting for async result of (UN)LOAD_SCRIPT_REQUESTED
    ExceptionsPanel@ epanel;
    float autosaveTimeCounterSec = 0.f;
    int autosaveResult = 0; // 0=no action, 1=success, -1=failure;

    // REQUEST 'QUEUE'
    string requestFoldRegion;
    string requestUnFoldRegion;
    bool requestFoldAll = false;
    bool requestUnFoldAll = false;
    bool requestIndentBuffer = false; // Only for invoking by user manually, not for use while saving!
    bool requestSaveFile = false;
    bool requestAutoSaveFile = false;
    bool requestRunBuffer = false;
    bool requestStopBuffer = false;

    ScriptEditorTab()
    {
        @epanel = ExceptionsPanel(this);
    }

    void drawStartStopButton()
    {
        if (this.waitingForManipEvent) // When waiting for async result of (UN)LOAD_SCRIPT_REQUESTED
        {
            ImGui::TextDisabled("WAIT...");
        }
        else
        {
            if (this.currentScriptUnitID == SCRIPTUNITID_INVALID)
            {
                if (ImGui::Button("[>>] RUN"))
                {
                    this.requestRunBuffer = true;
                }
            }
            else
            {
                if (ImGui::Button("[X] STOP"))
                {
                    this.requestStopBuffer = true;
                }
            }
        }
    }

    protected float measureLineInfoColumns()
    {
        // Reserve space for the metainfo columns
        float metaColumnsTotalWidth = 0.f;
        if (drawLineNumbers)
        metaColumnsTotalWidth += lineNumColumnWidth;
        if (drawLineLengths)
        metaColumnsTotalWidth += lineLenColumnWidth;
        if (drawLineCommentOffsets)
        metaColumnsTotalWidth += lineCommentColumnWidth;
        if (drawLineHttpOffsets)
        metaColumnsTotalWidth += lineHttpColumnWidth;
        errCountOffset = metaColumnsTotalWidth;
        if (drawLineErrCounts)
        metaColumnsTotalWidth += errCountColumnWidth;
        if (drawLineAutoIndentLevels)
        metaColumnsTotalWidth += lineAutoIndentLevelsColumnWidth;
        if (drawLineActualIndents)
        metaColumnsTotalWidth += lineActualIndentsColumnWidth;
        if (drawLineRegionMarkers)
        metaColumnsTotalWidth += lineRegionMarkerColumnWidth;

        return metaColumnsTotalWidth;
    }

    protected void drawLineInfoColumns(vector2 screenCursor)
    {
        ImGui::PushID("lineInfoCol");

        // draws columns with info (line number etc...) on the left side of the text area
        // ===============================================================================

        vector2 linenumCursorBase = screenCursor+vector2(0.f, -scrollY);
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();
        int bufferNumLines = int(this.bufferLinesMeta.length());

        float coarseClipBottomY = (screenCursor.y+textAreaSize.y)-ImGui::GetTextLineHeight()*0.6;
        float coarseClipTopY = screenCursor.y-ImGui::GetTextLineHeight()*0.7;

        bool inRegion = false;        // verified foldable region (`#endregion` was found -> has RegionInfo entry)
        bool drawRegionFoldBtn = false;   // verified foldable region
        string regionColumnStr = "";
        int nextLineNumber = 1; // Folded regions cause jumps in numbering

        for (int lineIdx = 0; lineIdx < bufferNumLines; lineIdx++)
        {
            vector2 linenumCursor = linenumCursorBase + vector2(0, lineIdx*ImGui::GetTextLineHeight());

            // Update line number; No drawing yet - must be done before coarse clipping
            // (Folded regions cause jumps in numbering)
            int currentLineNumber = nextLineNumber;
            if (drawLineNumbers)
            {
                int lineShift = 1;
                if (bool(this.bufferLinesMeta[lineIdx]['regionValid']))
                {
                    string regionName = string(this.bufferLinesMeta[lineIdx]['regionName']);
                    RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionName);
                    if (@regionInfo != null && regionInfo.isFolded)
                    {
                        lineShift = regionInfo.regionLineCount;
                    }
                }
                nextLineNumber += lineShift;
            }

            // Update code folding state; no drawing yet - must be done before coarse clipping
            if (drawLineRegionMarkers)
            {
                drawRegionFoldBtn = false;
                regionColumnStr = "";
                if (!inRegion)
                {
                    string regionName = string(this.bufferLinesMeta[lineIdx]['regionName']);
                    if (bool(bufferLinesMeta[lineIdx]['regionValid']))
                    {
                        // verify it's a foldable region (`#endregion` was found -> has RegionInfo entry)

                        RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionName);
                        RegionInfo@ fileRegionInfo = findRegion(this.workBufferRegions, regionName);
                        if (@regionInfo != null && @fileRegionInfo != null)
                        {
                            inRegion = true;
                            drawRegionFoldBtn = true;
                        }
                        else
                        {
                            regionColumnStr = "Br!"; // Broken ('#endregion' not found)
                        }
                    }
                    else if (bool(bufferLinesMeta[lineIdx]['regionFound']))
                    {
                        if (regionName == "")
                        {
                            regionColumnStr = "Un!"; // Unnamed
                        }
                        else
                        {
                            RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionName);
                            if (regionInfo.isOrphan)
                            {
                                regionColumnStr = "Or!"; // Orphan (duplicate)
                            }
                            else
                            {
                                regionColumnStr = "Ex!"; // Exists already
                            }
                        }
                    }
                }
                else // in foldable region
                {
                    if (bool(bufferLinesMeta[lineIdx]['endregionFound']))
                    {
                        inRegion = false;
                        regionColumnStr = " #~";
                    }
                    else
                    {
                        regionColumnStr = " | ";
                    }
                }
            }

            // Coarse clipping
            if (linenumCursor.y < coarseClipTopY)
            continue;
            if (linenumCursor.y > coarseClipBottomY)
            break;

            if (drawLineNumbers) // actually draw
            {
                drawlist.AddText(linenumCursor, lineNumberColor, ""+currentLineNumber);
                linenumCursor.x += lineNumColumnWidth;
            }

            if (drawLineLengths)
            {
                drawlist.AddText(linenumCursor, lineLenColor,
                ""+int(this.bufferLinesMeta[lineIdx]['len']));
                linenumCursor.x += lineLenColumnWidth;
            }

            if (drawLineCommentOffsets)
            {
                drawlist.AddText(linenumCursor, lineCommentOffsetColor,
                ""+int(this.bufferLinesMeta[lineIdx]['nonCommentLen']));
                linenumCursor.x += lineCommentColumnWidth;
            }

            if (drawLineHttpOffsets)
            {
                drawlist.AddText(linenumCursor, lineHttpOffsetColor,
                ""+int(this.bufferLinesMeta[lineIdx]['nonHttpLen']));
                linenumCursor.x += lineHttpColumnWidth;
            }

            // draw message count
            if (drawLineErrCounts)
            {

                if (bufferMessageIDs.length() > uint(lineIdx) // sanity check
                && bufferMessageIDs[lineIdx].length() > 0)
                {
                    // Draw num errors to the column

                    drawlist.AddText(linenumCursor, color(1.f,0.5f,0.4f,1.f), ""+bufferMessageIDs[lineIdx].length());
                }
                linenumCursor.x += errCountColumnWidth;
            }

            // Auto-detected indentation level, multiplied by preset indent width.
            if (drawLineAutoIndentLevels)
            {
                drawlist.AddText(linenumCursor, lineAutoIndentLevelsColor,
                ""+int(this.bufferLinesMeta[lineIdx]['autoIndentLevel']) * autoIndentNumSpaces);
                linenumCursor.x += lineAutoIndentLevelsColumnWidth;
            }

            // Actual indentation length
            if (drawLineActualIndents)
            {
                drawlist.AddText(linenumCursor, lineActualIndentsColor,
                ""+int(this.bufferLinesMeta[lineIdx]['actualIndentBlanks']));
                linenumCursor.x += lineActualIndentsColumnWidth;
            }

            // draw region&endregion markers
            if (drawLineRegionMarkers)
            {
                if (drawRegionFoldBtn)
                {
                    // Draw folding/unFolding button
                    string regionName = string(this.bufferLinesMeta[lineIdx]['regionName']);
                    RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionName);
                    if (@regionInfo != null) // Double check
                    {
                        vector2 cursorBackup = ImGui::GetCursorPos();
                        ImGui::SetCursorPos(linenumCursor - ImGui::GetWindowPos());
                        ImGui::PushStyleColor(ImGuiCol_Button, lineRegionMarkerColor);
                        ImGui::PushStyleColor(ImGuiCol_Text, color(0,0,0,1));
                        ImGui::PushID(lineIdx);
                        if (regionInfo.isFolded && ImGui::SmallButton("+"))
                        {
                            this.requestUnFoldRegion = regionName;
                        }
                        else if (!regionInfo.isFolded && ImGui::SmallButton("^"))
                        {
                            this.requestFoldRegion = regionName;
                        }
                        ImGui::PopID(); // lineIdx
                        ImGui::PopStyleColor(2); // ImGuiCol_Text, ImGuiCol_Button
                        ImGui::SetCursorPos(cursorBackup);
                    }
                }
                else if (regionColumnStr != "")
                {
                    drawlist.AddText(linenumCursor, lineRegionMarkerColor, regionColumnStr);
                }

                linenumCursor.x += lineRegionMarkerColumnWidth;
            }
        }

        ImGui::PopID(); //"lineInfoCol"
    }

    void drawTextAreaErrors()
    {
        // draw messages to text area
        //===========================

        vector2 msgCursorBase = this.errorsScreenCursor + vector2(4, -this.scrollY);

        int bufferNumLines = int(this.bufferLinesMeta.length());
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();

        for (int linenum = 1; linenum <= bufferNumLines; linenum++)
        {
            int lineIdx = linenum - 1;
            vector2 lineCursor = msgCursorBase  + vector2(0.f, ImGui::GetTextLineHeight()*lineIdx);

            // sanity checks
            if (this.bufferLinesMeta.length() <= uint(lineIdx))
            continue;

            // first draw comment highlights (semitransparent quads)
            //   special touch: if the comments are #[end]region markers, highlight them different.
            int startOffset = int(this.bufferLinesMeta[lineIdx]['startOffset']);
            int lineLen = int(this.bufferLinesMeta[lineIdx]['len']);
            int nonCommentLen = int(this.bufferLinesMeta[lineIdx]['nonCommentLen']);
            bool regionValid = bool(this.bufferLinesMeta[lineIdx]['regionValid']);
            string regionName = string(this.bufferLinesMeta[lineIdx]['regionName']);
            bool regionBroken = @findRegion(this.workBufferRegions, regionName) == null;
            bool endregionFound = bool(this.bufferLinesMeta[lineIdx]['endregionFound']);
            color rectColor = ((regionValid && !regionBroken) || endregionFound) ? regionCommentHighlightColor : commentHighlightColor;
            if (lineLen != nonCommentLen && drawCommentHighlights)
            {
                // calc pos
                string nonCommentStr = buffer.substr(startOffset, nonCommentLen);
                vector2 nonCommentSize = ImGui::CalcTextSize(nonCommentStr);
                vector2 pos = lineCursor + vector2(nonCommentSize.x, 0.f);

                // calc size
                int doubleslashCommentStart = int(this.bufferLinesMeta[lineIdx]['doubleslashCommentStart']);
                string commentStr = buffer.substr(doubleslashCommentStart, lineLen-nonCommentLen);
                vector2 commentSize = ImGui::CalcTextSize(commentStr);

                drawlist.AddRectFilled(pos, pos+commentSize, rectColor);
            }

            // second - http highlights
            int httpStart = int(this.bufferLinesMeta[lineIdx]['httpStart']);
            int httpLen = int(this.bufferLinesMeta[lineIdx]['httpLen']);
            if (httpStart >= 0 && httpLen >= 0 && drawHttpHighlights)
            {
                // calc pos
                int nonHttpLen = httpStart - startOffset;
                string nonHttpStr = buffer.substr(startOffset, nonHttpLen);
                vector2 nonHttpSize = ImGui::CalcTextSize(nonHttpStr);
                vector2 pos = lineCursor + vector2(nonHttpSize.x, 0.f);

                // calc size
                string httpStr = buffer.substr(httpStart, httpLen);
                vector2 httpSize = ImGui::CalcTextSize(httpStr);

                drawlist.AddRectFilled(pos, pos+httpSize, httpHighlightColor);

                // underline
                // FIXME: draw underline when hyperlink behaviours work here
                //        - The overlay window is 'NoInputs' so buttons don't work.
                //        - The base window doesn't receive clicks for some reason.
                //drawlist.AddLine(pos+vector2(0,httpSize.y), pos+httpSize, LINKCOLOR);
            }

            // sanity check
            if (this.bufferMessageIDs.length() <= uint(lineIdx))
            continue;

            // then errors and warnings
            for (uint i = 0; i < this.bufferMessageIDs[lineIdx].length(); i++)
            {
                uint msgIdx = this.bufferMessageIDs[lineIdx][i];

                int msgRow = int(this.messages[msgIdx]['row']);
                int msgCol = int(this.messages[msgIdx]['col']);
                int msgType = int(this.messages[msgIdx]['type']);
                string msgText = string(this.messages[msgIdx]['message']);
                string containingFoldedRegionName = string(this.messages[msgIdx]['inFoldedRegion']); // HACK: dynamic field inserted by `analyzeMessages()`

                color col, bgCol;
                bool shouldDraw = false;
                switch (msgType)
                {
                    case asMSGTYPE_ERROR:
                    col = errorTextColor;
                    bgCol = errorTextBgColor;
                    shouldDraw = drawErrorText;
                    break;
                    case asMSGTYPE_WARNING:
                    col = warnTextColor;
                    bgCol = warnTextBgColor;
                    shouldDraw = drawWarnText;
                    break;
                    default:;
                }

                if (!shouldDraw)
                continue;

                // Calc horizontal offset
                string subStr = buffer.substr(startOffset, msgCol-1);
                vector2 textPos = lineCursor + vector2(
                ImGui::CalcTextSize(subStr).x, // X offset - under the indicated text position
                ImGui::GetTextLineHeight()-2); // Y offset - slightly below the current line

                // Draw message text with background
                string errText = " ^ "+msgText;
                if (containingFoldedRegionName != "")
                {
                    // Special handling for folded regions - place message in-between lines, after region name (expect #endregion to be 'named' too!)
                errText = " } "+msgText;
                textPos += vector2(ImGui::CalcTextSize("// #***region "+containingFoldedRegionName).x, ImGui::GetTextLineHeight()/2);
            }
            vector2 errTextSize = ImGui::CalcTextSize(errText);
            drawlist.AddRectFilled(textPos, textPos+errTextSize, bgCol);
            drawlist.AddText(textPos , col, errText);

            // advance to next line
            lineCursor.y += ImGui::GetTextLineHeight();
        }
    }


}

void drawTextArea()
{

    // Reserve space for the metainfo columns
    float metaColumnsTotalWidth = measureLineInfoColumns();

    // Draw the multiline text input
    this.textAreaSize = vector2(
    ImGui::GetWindowSize().x - (metaColumnsTotalWidth + 20),
    ImGui::GetWindowSize().y - (115 + epanel.getHeightToReserve()));

    vector2 screenCursor = ImGui::GetCursorScreenPos() + /*frame padding:*/vector2(0.f, 4.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + metaColumnsTotalWidth);
    bool changed = ImGui::InputTextMultiline(BUFFER_TEXTINPUT_LABEL, this.buffer, textAreaSize);

    // Trick: re-enter the inputtext panel to scroll it; see https://github.com/ocornut/imgui/issues/1523
    ImGui::BeginChild(ImGui::GetID(BUFFER_TEXTINPUT_LABEL));
    this.scrollX = ImGui::GetScrollX();
    this.scrollY = ImGui::GetScrollY();
    this.scrollMaxX = ImGui::GetScrollMaxX();
    this.scrollMaxY = ImGui::GetScrollMaxY();
    ImGui::EndChild();

    if (changed)
    {
        this.analyzeLines();
        // Enlarge buffer if at capacity
        if (this.totalChars == (this.buffer.length() - 1))
        this.buffer.resize(this.buffer.length() + BUFFER_INCREMENT_SIZE);
    }

    this.drawLineInfoColumns(screenCursor);
    screenCursor.x+= metaColumnsTotalWidth;
    // Apparently drawing errors now makes them appear under the editor text;
    // we must remember values and draw separately.
    this.errorsScreenCursor = screenCursor;
    this.errorsTextAreaSize = textAreaSize;

}

void drawExceptionsPanel()
{

    if (epanel.getHeightToReserve() == 0)
    {
        return; // panel is empty, do not draw
    }

    // Draw the exceptions panel
    vector2 ePanelSize(
    ImGui::GetWindowSize().x - (20),
    epanel.getHeightToReserve());
    ImGui::BeginChild("ExceptionsPanel", ePanelSize);
    epanel.drawExceptions();
    ImGui::EndChild();    // must always be called - legacy reasons
}

void drawFooter()
{

    if (epanel.getHeightToReserve() == 0)
    {
        // make top gap (window padding) rougly same size as bottom gap (item spacing)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+2);
    }
    else
    {
        // Fix footer jumping down (??)
        ImGui::SetCursorPosY(ImGui::GetCursorPosY()-3);
    }

    // footer with status info
    ImGui::Separator();
    string runningTxt = "NOT RUNNING";
    if (this.currentScriptUnitID != SCRIPTUNITID_INVALID)
    {
        runningTxt = "RUNNING (NID: "+this.currentScriptUnitID+")";
    }
    ImGui::Text(runningTxt
    +"; lines: "+bufferLinesMeta.length()
    +" (chars: "+this.totalChars+"/"+(this.buffer.length()-1)+")" // Leave buffer space for one '\0'
    +"; messages:"+this.messages.length()
    +" (err:"+this.messagesTotalErr
    +"/warn:"+this.messagesTotalWarn
    +"/info:"+this.messagesTotalInfo
    +"); scrollY="+scrollY+"/"+scrollMaxY+"");    // X="+scrollX+"/"+scrollMaxX
}

void setBuffer(string data)
{
    // Make sure buffer is no smaller than BUFFER_MIN_SIZE
    // and make sure it has at least BUFFER_INCREMENT_SIZE extra space.
    // ===============================================================
    this.buffer = data;
    this.buffer.resize(this.buffer.length() + BUFFER_INCREMENT_SIZE);
    if (this.buffer.length() < BUFFER_MIN_SIZE)
    this.buffer.resize(BUFFER_MIN_SIZE);
    this.analyzeLines();
    editorWindow.refreshLocalFileList();
}

private void runBufferInternal() // do not invoke during drawing ~ use `requestRunBuffer`
{
    this.bufferMessageIDs.resize(0); // clear all
    this.messages.resize(0); // clear all
    this.epanel.clearExceptions();

    this.backUpRegionFoldStates();
    this.unFoldAllRegionsInternal(); // OK to call here - we're already handling a request.

    game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED, {
    {'filename', this.bufferName}, // Because we supply the buffer, this will serve only as display name
    {'buffer', this.buffer },
    {'category', SCRIPT_CATEGORY_CUSTOM}
    });
    waitingForManipEvent=true;

    this.restoreRegionFoldStates();
}

private void stopBufferInternal() // do not invoke during drawing ~ use `requestStopBuffer`
{
    game.pushMessage(MSG_APP_UNLOAD_SCRIPT_REQUESTED, {
    {'id', this.currentScriptUnitID}
    });
    waitingForManipEvent=true;
}

void onEventAngelScriptMsg(int scriptUnitId, int msgType, int row, int col, string sectionName, string message)
{
    /*game.log("DBG '"+this.bufferName+"' onEventAngelScriptMsg(): scriptUnitId:" + scriptUnitId
    + ", msgType:" + msgType + ", row:" + row + ", col:" + col // ints
    + ", sectionName:" + sectionName + ", message:" + message); // strings*/

messages.insertLast({ {'type', msgType}, {'row',row}, {'col', col}, {'sectionName', sectionName}, {'message', message} });
    this.analyzeMessages();
}

void onEventAngelScriptManip(int manipType, int scriptUnitId, int scriptCategory, string scriptName)
{
    /*game.log ("DBG '"+this.bufferName+"'.onEventAngelScriptManip(): manipType:"+ manipType+", scriptUnitId:"+ scriptUnitId
    + ", scriptCategory:"+scriptCategory+", scriptName:"+scriptName+"; //this.waitingForManipEvent:"+this.waitingForManipEvent);*/

    // Only handle LOADED manip if we're waiting for it
    if (manipType == ASMANIP_SCRIPT_LOADED
    && this.currentScriptUnitID == SCRIPTUNITID_INVALID
    && this.bufferName == scriptName
    && this.waitingForManipEvent)
    {
        this.currentScriptUnitID = scriptUnitId;
        waitingForManipEvent = false;
        // Force registering of exception callback so that the editor can monitor exceptions.
        game.setRegisteredEventsMask(scriptUnitId,
        game.getRegisteredEventsMask(scriptUnitId) | SE_ANGELSCRIPT_EXCEPTIONCALLBACK);
        /*game.log ("DBG '"+this.bufferName+"'.onEventAngelScriptManip(): Now running with NID="+this.currentScriptUnitID);*/
    }
    // Handle UNLOADING manip even if not expected - user may abort script manually via Console/ScriptMonitorUI or it may have crashed and get killed by the editor (see `killScriptOnAngelscriptException`).
    else if (manipType == ASMANIP_SCRIPT_UNLOADING
    && this.currentScriptUnitID == scriptUnitId
    && this.bufferName == scriptName)
    {
        /*game.log ("DBG '"+this.bufferName+"'.onEventAngelScriptManip(): Stopping, was using NID="+this.currentScriptUnitID);*/
        this.currentScriptUnitID = SCRIPTUNITID_INVALID;
        waitingForManipEvent = false;
    }

}

void analyzeLines()
{
    this.analyzeBuffer();
    this.analyzeMessages();
}

private void analyzeBuffer() // helper for `analyzeLines()`
{
    /*
    // [profiling]
    Ogre::Timer timer;
    uint totalMicrosecDict = 0; // the `bufferLinesMeta` array of dicts
    uint totalMicrosecCpuDict = 0;
    uint totalMicrosecRegions = 0;
    uint totalMicrosecCpuRegions = 0;
    */

    this.bufferLinesMeta.resize(0); // clear all
    int startOffset = 0;
    this.totalChars = 0;

    // pattern - comment
    string commentPattern = "//";
    uint commentLevel = 0;
    int commentStart = -1;
    bool commentFound = false;

    // pattern - hyperlink
    string httpPattern = 'http://';
    uint httpLevel = 0;
    int httpStart = -1;
    bool httpFound = false;

    // pattern - #region & #endregion (C#-like, inside comment)
    // NOTE: Regions cannot be nested. '#[end]region' lines are always present in `buffer` even if folded.
    string regionPattern = '#region';
    uint regionLevel = 0;
    int regionTitleStart = -1;
    bool regionFound = false;
    string endregionPattern = '#endregion';
    uint endregionLevel = 0;
    bool endregionFound = false;
    // Code folding
    dictionary collectedRegions;
    int regionFoundAtLineIdx = -1;
    string regionFoundWithName;
    int regionBodyStartOffset = -1;

    string httpBreakChars = " \n\t\"'";
    bool httpBreakFound = false;
    int httpBreakPos = -1;

    int autoIndentLevelCurrent = 0;
    int autoIndentLevelNext = 0;
    int actualIndentBlanks = 0; // current line
    bool actualIndentFound = false;

string SPECIALCHARS = "{}\n\t \0";

    for (uint i = 0; i < this.buffer.length(); i++)
    {
        uint lineIdx = this.bufferLinesMeta.length();

        // analyze the character
        uint theChar = this.buffer[i];
        bool isCharOpenBrace = theChar == SPECIALCHARS[0];
        bool isCharCloseBrace = theChar == SPECIALCHARS[1];
        bool isCharNewline = theChar == SPECIALCHARS[2];
        bool isCharTab = theChar == SPECIALCHARS[3];
        bool isCharSpace = theChar == SPECIALCHARS[4];
        bool isCharNul = theChar == SPECIALCHARS[5];

        // doubleslash comment
        if (!commentFound)
        {
            if (this.buffer[i] == commentPattern[commentLevel]
            && (httpStart == -1 || httpBreakFound)) // Either we're not in hyperlink yet or we're after it already.)
            {
                commentLevel++;

                // Record the '//', but make sure it's not within hyperlink!
                if (commentStart == -1)
                {
                    commentStart = i;
                }

                if (uint(commentLevel) == commentPattern.length())
                {
                    commentFound = true;
                }
            }
            else
            {
                commentLevel = 0;
            }
        }
        else
        {
            if (!regionFound)
            {
                //if (regionLevel > 0) game.log("DBG analyzeBuffer(): char="+i+", line="+lineIdx+", regionLevel="+regionLevel+"/"+regionPattern.length());
                if (this.buffer[i] == regionPattern[regionLevel])
                {
                    regionLevel++;
                    if (uint(regionLevel) == regionPattern.length())
                    {
                        regionFound = true;
                        regionTitleStart = i+1;
                    }
                }
                else
                {
                    regionLevel = 0;
                }
            }

            if (!endregionFound)
            {
                //if (endregionLevel > 0) game.log("DBG analyzeBuffer(): char="+i+", line="+lineIdx+", endregionLevel="+endregionLevel+"/"+endregionPattern.length());
                if (this.buffer[i] == endregionPattern[endregionLevel])
                {
                    endregionLevel++;
                    if (uint(endregionLevel) == endregionPattern.length())
                    {
                        endregionFound = true;
                    }
                }
                else
                {
                    endregionLevel = 0;
                }
            }
        }

        // http protocol
        if (!httpFound)
        {
            if (this.buffer[i] == httpPattern[httpLevel])
            {
                httpLevel++;

                if (httpStart == -1)
                {
                    httpStart = i;
                }

                if (uint(httpLevel) >= httpPattern.length())
                {
                    httpFound = true;
                }
            }
            else
            {
                httpLevel = 0;
            }
        }
        else if (!httpBreakFound)
        {
            for (uint j = 0; j < httpBreakChars.length(); j++)
            {
                if (this.buffer[i] == httpBreakChars[j])
                {
                    httpBreakFound = true;
                    httpBreakPos = i;
                }
            }
        }

        // Auto indentation - always execute to display the debug levels
        if (isCharOpenBrace)
        {
            autoIndentLevelNext++;
        }
        else if (isCharCloseBrace)
        {
            // Allow negative values, for debugging.
            autoIndentLevelNext--;
            autoIndentLevelCurrent--;
        }

        // Actual indentation - always execute
        if (!actualIndentFound)
        {
            if (isCharSpace || isCharTab)
            {
                actualIndentBlanks++;
            }
            else
            {
                actualIndentFound = true;
            }
        }

        if (isCharNewline || isCharNul)
        {
            int endOffset = i;

            /*
            // [Profiling]
            uint microsecBeforeRegions = timer.getMicroseconds();
            uint microsecCpuBeforeRegions = timer.getMicrosecondsCPU();
            */

            // Process #region
            if (regionFound)
            {
                regionFoundAtLineIdx = lineIdx;
                regionFoundWithName = trimLeft(this.buffer.substr(regionTitleStart, endOffset - regionTitleStart));
                regionBodyStartOffset = endOffset+1;
                //game.log("DBG analyzeBuffer(): regionFound: withName="+regionFoundWithName+" atLineIdx="+regionFoundAtLineIdx+" bodyStartOffset="+regionBodyStartOffset);
            }

            // Process #endregion
            if (endregionFound && regionFoundAtLineIdx != -1 && !collectedRegions.exists(regionFoundWithName))
            {
                RegionInfo regionInfo;
                regionInfo.regionLineCount = (lineIdx-1)-regionFoundAtLineIdx; // To handle jumps in line numbering
                regionInfo.regionBodyStartOffset = regionBodyStartOffset; // To swap regions in and out from work buffer.
                regionInfo.regionBodyNumChars = (int(this.bufferLinesMeta[lineIdx-1]['endOffset']) - regionBodyStartOffset)+1; // ditto
                regionInfo.regionStartsAtLineIndex = regionFoundAtLineIdx;
                collectedRegions[regionFoundWithName] = regionInfo;
                //game.log("DBG analyzeBuffer(): endregionFound: withName="+regionFoundWithName+" lineCount="+regionInfo.regionLineCount+" bodyStartOffset="+regionInfo.regionBodyStartOffset+" numChars="+regionInfo.regionBodyNumChars);
                regionFoundAtLineIdx = -1;
                regionFoundWithName = "";
            }

            /*
            // [Profiling]
            totalMicrosecRegions += timer.getMicroseconds() - microsecBeforeRegions;
            totalMicrosecCpuRegions += timer.getMicrosecondsCPU() - microsecCpuBeforeRegions;
            // Finish line
            uint microsecBeforeDict = timer.getMicroseconds();
            uint microsecCpuBeforeDict = timer.getMicrosecondsCPU();
            */

            this.bufferLinesMeta.insertLast({ {'startOffset', startOffset} });
            this.bufferLinesMeta[lineIdx]['endOffset'] = endOffset;
            int len = endOffset - startOffset;
            this.bufferLinesMeta[lineIdx]['len'] = len;
            this.bufferLinesMeta[lineIdx]['doubleslashCommentStart'] = commentStart;
            int nonCommentLen = (commentFound && commentStart >= 0) ? commentStart - startOffset : len;
            this.bufferLinesMeta[lineIdx]['nonCommentLen'] = nonCommentLen;
            this.bufferLinesMeta[lineIdx]['httpStart'] = (httpFound) ? httpStart : -1;
            this.bufferLinesMeta[lineIdx]['httpLen'] = (httpFound) ? httpBreakPos-httpStart : -1;
            this.bufferLinesMeta[lineIdx]['nonHttpLen'] = (httpFound) ? httpStart - startOffset : -1;
            this.bufferLinesMeta[lineIdx]['regionFound'] = regionFound;
            this.bufferLinesMeta[lineIdx]['regionValid'] = regionFound && regionFoundWithName != "" && !collectedRegions.exists(regionFoundWithName);
            this.bufferLinesMeta[lineIdx]['regionName'] = regionFoundWithName;
            this.bufferLinesMeta[lineIdx]['endregionFound'] = endregionFound;
            this.bufferLinesMeta[lineIdx]['autoIndentLevel'] = autoIndentLevelCurrent;
            this.bufferLinesMeta[lineIdx]['actualIndentBlanks'] = actualIndentBlanks;

            /*
            // [Profiling]
            totalMicrosecDict += timer.getMicroseconds() - microsecBeforeDict;
            totalMicrosecCpuDict += timer.getMicrosecondsCPU() - microsecCpuBeforeDict;
            */
        }

        if (isCharNul)
        {
            break; // We're done parsing the buffer - the rest is just more NULs.
        }
        else if (isCharNewline)
        {
            // reset context
            commentFound = false;
            commentStart = -1; // -1 means Empty
            commentLevel = 0;

            httpFound = false;
            httpStart = -1; // -1 means Empty
            httpLevel = 0;
            httpBreakFound = false;
            httpBreakPos = -1; // -1 means Empty

            regionFound = false;
            regionTitleStart = -1; // -1 means Empty
            regionLevel = 0;

            endregionFound = false;
            endregionLevel = 0;

            actualIndentFound = false;
            actualIndentBlanks = 0;

            autoIndentLevelCurrent = autoIndentLevelNext;

            // Start new line
            startOffset = i+1;
        }

        this.totalChars++;
    }

    /*
    // [profiling]
    uint microsecBeforeMerging = timer.getMicroseconds();
    uint microsecCpuBeforeMerging = timer.getMicrosecondsCPU();
    */

    this.mergeCollectedFoldingRegionsWithExisting(collectedRegions);

    /*
    // [profiling]
    uint totalMicrosecMerging = timer.getMicroseconds() - microsecBeforeMerging;
    uint totalMicrosecCpuMerging = timer.getMicrosecondsCPU() - microsecCpuBeforeMerging;
    game.log("PROFILING analyzeBuffer() [in microseconds]:"
    +" total "+timer.getMicroseconds()+"us (CPU "+timer.getMicrosecondsCPU()+"us)"
    +" regions "+totalMicrosecRegions+"us (CPU "+totalMicrosecCpuRegions+"us)"
    +" dict "+totalMicrosecDict+"us (CPU "+totalMicrosecCpuDict+"us)"
    +" merging "+totalMicrosecMerging+"us (CPU "+totalMicrosecCpuMerging+"us)"
    );
    */
}

private void mergeCollectedFoldingRegionsWithExisting(dictionary&in collectedRegions) // helper for `analyzeBuffer()`
{
    // The `ImGui::TextInputMultiline()` doesn't provide info on where new characters were inserted/removed (or does it? TODO research ImGuiInputTextCallbackData)
    // Either way, we simply scan the buffer again, collect the regions and then match them with those already existing.
    // This relies on the #region/#endregion tags always being in the buffer, even if folded.
    // ---------------------------------------------------------------------------------------------------------------

    // prune broken regions (missing '#endregion' -> RegionInfo is null)
    array<string>@ collectedRegionNames = collectedRegions.getKeys();
    for (uint i = 0; i< collectedRegionNames.length(); i++)
    {
        RegionInfo@ regionInfo = findRegion(collectedRegions, collectedRegionNames[i]);
        if (@regionInfo == null)
        {
            //game.log ("DBG mergeCollectedFoldingRegionsWithExisting(): pruning broken region '" + collectedRegionNames[i] + "'");
            collectedRegions.delete(collectedRegionNames[i]);
        }
    }

    // Find regions that were deleted/changed
    array<string> oldRegionNames = this.workBufferRegions.getKeys();
    for (uint i = 0; i< oldRegionNames.length(); i++)
    {
        bool isGone = !collectedRegions.exists(oldRegionNames[i]);
        RegionInfo@ oldRegionInfo = findRegion(this.workBufferRegions, oldRegionNames[i]);
        if (isGone && oldRegionInfo.isFolded)
        {
            //game.log ("DBG mergeCollectedFoldingRegionsWithExisting(): region '" + oldRegionNames[i] + "' has gone orphan.");
            oldRegionInfo.isOrphan = true;
        }
    }

    // Find regions that were (re)created
    array<string>@ newRegionNames = collectedRegions.getKeys();
    for (uint i = 0; i < newRegionNames.length(); i++)
    {
        RegionInfo@ newRegionInfo = findRegion(collectedRegions, newRegionNames[i]);
        RegionInfo@ existingRegionInfo = findRegion(this.workBufferRegions, newRegionNames[i]);
        if (@existingRegionInfo == null)
        {
            //game.log("DBG mergeCollectedFoldingRegionsWithExisting(): A brand new region '"+newRegionNames[i]+"' was created");
            this.workBufferRegions[newRegionNames[i]] = newRegionInfo;
        }
        else
        {
            /*game.log("DBG mergeCollectedFoldingRegionsWithExisting(): Region '"+newRegionNames[i]+"' already exists:"
            +" lineCount="+existingRegionInfo.regionLineCount+" (new:"+newRegionInfo.regionLineCount+")"
            +" regionBodyStartOffset="+existingRegionInfo.regionBodyStartOffset+" (new:"+newRegionInfo.regionBodyStartOffset+")"
            +" regionBodyNumChars="+existingRegionInfo.regionBodyNumChars+" (new:"+newRegionInfo.regionBodyNumChars+")"
            +" isOrphan="+existingRegionInfo.isOrphan+" isFolded="+newRegionInfo.isFolded);
            */

            existingRegionInfo.regionBodyStartOffset = newRegionInfo.regionBodyStartOffset;

            if (!existingRegionInfo.isFolded)
            {
                //game.log("DBG mergeCollectedFoldingRegionsWithExisting(): An existing UNFOLDED region '"+newRegionNames[i]+"' was updated ~ text may have changed");
                existingRegionInfo.regionLineCount = newRegionInfo.regionLineCount;
                existingRegionInfo.regionBodyNumChars = newRegionInfo.regionBodyNumChars;
                existingRegionInfo.regionStartsAtLineIndex = newRegionInfo.regionStartsAtLineIndex;
            }
            else if (existingRegionInfo.isOrphan && newRegionInfo.regionLineCount == 0)
            {
                //game.log("DBG mergeCollectedFoldingRegionsWithExisting(): An orphan (so logically FOLDED) region '"+newRegionNames[i]+"' has resurfaced");
                existingRegionInfo.isOrphan = false;
            }
        }
    }
}

private void analyzeMessages() // helper for `analyzeLines()`
{
    // reset caches
    bufferMessageIDs.resize(0); // clear all
    bufferMessageIDs.resize(bufferLinesMeta.length());

    // reset global stats
    this.messagesTotalErr = 0;
    this.messagesTotalWarn = 0;
    this.messagesTotalInfo = 0;


    for (uint i = 0; i < this.messages.length(); i++)
    {
        int msgType = int(this.messages[i]['type']);
        string msgText = string(this.messages[i]['message']);
        string inFoldedRegion = "";
        uint lineIdx = this.determineLineIdxForMessage(i, /*[out]:*/inFoldedRegion);
        this.messages[i]['inFoldedRegion'] = inFoldedRegion; // HACK: add extra dynamic field where only constant data from AngelScript should be

        // BEGIN Check bounds (investigating exceptions)
        if (msgType == asMSGTYPE_ERROR || msgType == asMSGTYPE_WARNING)
        {
            if (i >= bufferMessageIDs.length())
            {
                game.log("DBG WARNING: analyzeMessages() `i` out of bounds, breaking loop (seems to happen each time new build errors arrive and there's more than last time).");
                game.log("DBG   analyzeMessages() | i="+i+", Looping over this.messages.length()="+this.messages.length()+", writing to bufferMessageIDs.length()="+ bufferMessageIDs.length());
                game.log ("DBG   analyzeMessages() | msgType is: ERROR=" + (msgType == asMSGTYPE_ERROR) + " / WARNING = " + (msgType == asMSGTYPE_WARNING));

                break;
            }

            // Check bounds (investigating a rare exception)
            if (lineIdx >= bufferMessageIDs.length())
            {
                game.log("DBG WARNING: analyzeMessages() `lineIdx` out of bounds, breaking loop.");
                game.log("DBG   analyzeMessages() |  i="+i+", using lineIdx="+lineIdx+" to write into  bufferMessageIDs.length()="+ bufferMessageIDs.length());
                game.log ("DBG   analyzeMessages() | msgType is: ERROR=" + (msgType == asMSGTYPE_ERROR) + " / WARNING = " + (msgType == asMSGTYPE_WARNING));

                break;
            }
        }
        // END bounds check

        // update line stats
        if (msgType == asMSGTYPE_ERROR)
        {
            this.bufferMessageIDs[lineIdx].insertLast(i);
            this.messagesTotalErr++;
        }
        if (msgType == asMSGTYPE_WARNING)
        {
            this.bufferMessageIDs[lineIdx].insertLast(i);
            this.messagesTotalWarn++;
        }
        if (msgType == asMSGTYPE_INFORMATION)
        {
            this.messagesTotalInfo++;
        }
    }
}

private uint determineLineIdxForMessage(uint msgIndex, string&inout inFoldedRegion) // helper for `analyzeMessages()`
{
    // AngelScript reports line numbers from complete file, but `this.buffer` doesn't contain folded regions so line indices in `this.bufferLinesMeta` don't match.
    // To determine the message's lineindex in the sparse `this.buffer`, we must go through region records and adjust the reported line number.
    // Note the `this.workBufferRegions` set isn't ordered, we must process all regions that precede the message, and only then the region which contains it (if any).
    // --------------------------------------------------------------------------------------------------------------------------------------------------------

    int msgRow = int(this.messages[msgIndex]['row']);
    int lineIdx = msgRow-1;

    //game.log("DBG determineLineIdxForMessage("+msgIndex+"): initial lineIdx="+lineIdx);

    array<string> regionKeys = this.workBufferRegions.getKeys();
    RegionInfo@ containingRegion = null;
    string containingRegionName = "";
    for (uint i = 0; i < regionKeys.length(); i++)
    {
        RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionKeys[i]);
        if (@regionInfo != null)
        {
            //game.log("DBG determineLineIdxForMessage("+msgIndex+"): considering region "+i+"/"+regionKeys.length()+" '"+regionKeys[i]+"' (isFolded="+regionInfo.isFolded+" startsAtLineIndex="+regionInfo.regionStartsAtLineIndex+" lineCount="+regionInfo.regionLineCount+")");

            bool lowerBoundReached = (regionInfo.regionStartsAtLineIndex < lineIdx);
            bool upperBoundReached = ((regionInfo.regionStartsAtLineIndex + regionInfo.regionLineCount) < lineIdx);

            if (regionInfo.isFolded)
            {
                if (!lowerBoundReached)
                {
                    // This region precedes the message
                    lineIdx -= regionInfo.regionLineCount;
                }
                else if (lowerBoundReached && !upperBoundReached)
                {
                    // This region contains the message - we must account it last to stay in sync.
                    @containingRegion = @regionInfo;
                    containingRegionName = regionKeys[i];
                }
            }

            //game.log("DBG ... lowerBoundReached:"+lowerBoundReached+" upperBoundReached:"+upperBoundReached+" <is it containingRegion?>="+(@containingRegion == @regionInfo)+"; new lineIdx="+lineIdx);
        }
    }

    if (@containingRegion!=null && containingRegion.isFolded)
    {
        lineIdx -= 1 + (lineIdx - containingRegion.regionStartsAtLineIndex);
        inFoldedRegion = containingRegionName;
    }

    // workaround: clamp the msgRow to known line count
    if (lineIdx >= int(bufferLinesMeta.length()))
    {
        lineIdx = int(bufferLinesMeta.length()) - 1;
    }

    //game.log("DBG determineLineIdxForMessage("+msgIndex+"): final lineIdx="+lineIdx);
    return lineIdx;
}

void updateAutosave(float dt)
{
    if (autoSaveIntervalSec == 0.f)
    return; // disabled

    this.autosaveTimeCounterSec+= dt;
    this.autosaveResult = 0;
    if (this.autosaveTimeCounterSec > autoSaveIntervalSec)
    {
        this.requestAutoSaveFile = true;
        this.autosaveTimeCounterSec -= autoSaveIntervalSec; // don't miss a millisecond!
    }
}

void drawAutosaveStatusbar()
{
    ImGui::TextDisabled("Autosave:");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, autoSaveStatusbarBgColor);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()+5.f);
    ImGui::ProgressBar(
    /*fraction:*/this.autosaveTimeCounterSec/autoSaveIntervalSec,
    /*size: */vector2(55.f, 13.f),
    /*overlay: */''+formatFloat(autoSaveIntervalSec-this.autosaveTimeCounterSec, "", 3, 1)+'sec');
    ImGui::PopStyleColor(1); // ImGuiCol_FrameBg
}

void handleRequests(float dt)
{
    if (this.requestFoldRegion != "")
    {
        this.foldRegionInternal(this.requestFoldRegion);
        this.requestFoldRegion = "";
    }
    else if (this.requestUnFoldRegion != "")
    {
        //game.log("DBG requestUnFoldRegion: '"+ this.requestUnFoldRegion+"'");
        this.unFoldRegionInternal(this.requestUnFoldRegion);
        this.requestUnFoldRegion = "";
    }
    else if (this.requestFoldAll)
    {
        this.foldAllRegionsInternal();
        this.requestFoldAll = false;
    }
    else if (this.requestUnFoldAll)
    {
        //game.log("DBG requestUnfoldAll:"+this.requestUnFoldAll);
        this.unFoldAllRegionsInternal();
        this.requestUnFoldAll = false;
    }
    if (this.requestIndentBuffer)
    {
        this.indentBufferInternal();
        this.requestIndentBuffer = false;
    }
    if (this.requestSaveFile)
    {
        this.saveFileInternal();
        this.requestSaveFile = false;
    }
    if (this.requestAutoSaveFile)
    {
        this.autosaveFileInternal();
        this.requestAutoSaveFile = false;
    }
    if (this.requestRunBuffer)
    {
        this.runBufferInternal();
        this.requestRunBuffer = false;
    }
    if (this.requestStopBuffer)
    {
        this.stopBufferInternal();
        this.requestStopBuffer = false;
    }
}

private void saveFileInternal() // Do not invoke while drawing! use `requestSaveFile`
{
    // perform auto-indenting if desired
    if (autoIndentOnSave)
    this.indentBufferInternal(); // OK to call here - we're already handing a request.

    this.backUpRegionFoldStates();
    this.unFoldAllRegionsInternal(); // OK to call here - we're already handling a request.

    // Write out the file
    string strData = this.buffer.substr(0, this.totalChars);
    bool savedOk = game.createTextResourceFromString(
    strData, editorWindow.saveFileNameBuf, RGN_SCRIPTS, saveShouldOverwrite);
    editorWindow.saveFileResult = savedOk ? 1 : -1;
    if (savedOk)
    {
        editorWindow.addRecentScript(editorWindow.saveFileNameBuf);
    }

    this.restoreRegionFoldStates();
}

private void autosaveFileInternal() // Do not invoke while drawing! use `requestAutoSaveFile`
{
    this.backUpRegionFoldStates();
    this.unFoldAllRegionsInternal(); // OK to call here - we're already handling a request.

// Generate filename: ${orig_name}_AUTOSAVE.$(orig_ext)
    int dotPos = bufferName.findLast(".");
    string autosaveFilename;
    if (dotPos > 0)
    {
        autosaveFilename = bufferName.substr(0, dotPos) + "_AUTOSAVE" + bufferName.substr(dotPos);
    }
    else
    {
        autosaveFilename = bufferName + "_AUTOSAVE";
    }
    /*game.log ("DBG autosaveFileInternal: autosaveFilename='"+autosaveFilename+"'");*/

    // Write out the file
    string strData = this.buffer.substr(0, this.totalChars);
    bool savedOk = game.createTextResourceFromString(
    strData, autosaveFilename, RGN_SCRIPTS, /*overwrite:*/true);
    this.autosaveResult = savedOk ? 1 : -1;
    this.restoreRegionFoldStates();
}

private void indentBufferInternal() // Do not invoke while drawing! use `requestIndentBuffer`
{
    this.backUpRegionFoldStates();
    this.unFoldAllRegionsInternal();

    string stagingBuffer;
    for (uint i = 0; i < bufferLinesMeta.length(); i++)
    {
        // insert indent
        int autoIndentLevel = int(bufferLinesMeta[i]['autoIndentLevel']);
    for (int j=0; j<autoIndentNumSpaces*autoIndentLevel; j++) { stagingBuffer += " "; }

        // insert the rest of line
        int startOffset = int(bufferLinesMeta[i]['startOffset']);
        int endOffset = int(bufferLinesMeta[i]['endOffset']);
        int actualIndentBlanks = int(bufferLinesMeta[i]['actualIndentBlanks']);
        stagingBuffer += this.buffer.substr(startOffset + actualIndentBlanks, endOffset - (startOffset + actualIndentBlanks));

        // Insert newline, but don't put any extra at the end of file
        if (i < bufferLinesMeta.length() - 1 || !isChar(stagingBuffer[stagingBuffer.length() - 1], '\n')) // if (not at EOF yet || at EOF but there's no newline) ...
        {
            stagingBuffer += '\n';
        }
    }

    // submit buffer
    this.totalChars = stagingBuffer.length();
    this.buffer = stagingBuffer;

    // reserve extra space
    this.buffer.resize(this.buffer.length() + BUFFER_INCREMENT_SIZE);

    // Update the lines metainfo
    this.analyzeBuffer();

    this.restoreRegionFoldStates(); // restore backup
    this.requestIndentBuffer = false;
}

private void foldAllRegionsInternal() // do NOT invoke during drawing! use `requestFoldAll`
{
    array<string> regionNames = this.workBufferRegions.getKeys();
    for (uint i=0; i < regionNames.length(); i++)
    {
        this.foldRegionInternal(regionNames[i]);
    }

}

private void unFoldAllRegionsInternal() //  do NOT invoke during drawing! use `requestUnFoldAll`
{
    array<string> regionNames = this.workBufferRegions.getKeys();
    for (uint i=0; i < regionNames.length(); i++)
    {
        this.unFoldRegionInternal(regionNames[i]);
    }
    this.requestUnFoldAll = false;
}

private void foldRegionInternal(string regionName)  // do NOT invoke during drawing! use `foldRegionRequested`
{
    RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionName);
    /*
    game.log("DBG foldRegionInternal() regionName='"+regionName+"', regionInfo:"+(@regionInfo == null ? "null" :
    "NumChars:"+regionInfo.regionBodyNumChars+", isFolded:"+regionInfo.isFolded+", regionBodyStartOffset:"+regionInfo.regionBodyStartOffset+", regionBodyNumChars:"+regionInfo.regionBodyNumChars));
    */
    if (@regionInfo != null && !regionInfo.isFolded) // sanity check - this means `#endregion` isn't available
    {
        regionInfo.foldedOffText = this.buffer.substr(regionInfo.regionBodyStartOffset, regionInfo.regionBodyNumChars);
        this.buffer.erase(regionInfo.regionBodyStartOffset, regionInfo.regionBodyNumChars);
        regionInfo.isFolded = true;
        this.analyzeLines();
        this.analyzeMessages(); // Determine which errors are in folded region, for correct drawing
    }
}

private void unFoldRegionInternal(string regionName) //  do NOT invoke during drawing! use `unFoldRegionRequested`
{
    RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionName);
    /*game.log("DBG unFoldRegionInternal() regionName='"+regionName+"', regionInfo:"+(@regionInfo == null ? "null" :
    "NumChars:"+regionInfo.regionBodyNumChars+", isFolded:"+regionInfo.isFolded+", regionBodyStartOffset:"+regionInfo.regionBodyStartOffset+", regionBodyNumChars:"+regionInfo.regionBodyNumChars));
    */
    if (@regionInfo != null && regionInfo.isFolded) // sanity check - this means `#endregion` isn't available
    {
        this.buffer.insert(regionInfo.regionBodyStartOffset, regionInfo.foldedOffText);
        regionInfo.foldedOffText = "";
        regionInfo.isFolded = false;
        this.analyzeLines();
        this.analyzeMessages(); // Determine which errors are in folded region, for correct drawing
    }
}

private void backUpRegionFoldStates()
{
    //game.log("DBG backUpRegionFoldStates()");
    array<string> regionNames = this.workBufferRegions.getKeys();
    for (uint i=0; i < regionNames.length(); i++)
    {
        RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionNames[i]);
        if (@regionInfo != null)
        {
            regionInfo.isFoldedBackup = regionInfo.isFolded;
            //game.log("DBG backUpRegionFoldStates() ~ region '"+regionNames[i]+"': isFolded="+regionInfo.isFolded+" (backup: "+regionInfo.isFoldedBackup+")");
        }
    }
}

private void restoreRegionFoldStates()
{
    //game.log("DBG restoreRegionFoldStates()");
    array<string> regionNames = this.workBufferRegions.getKeys();
    for (uint i=0; i < regionNames.length(); i++)
    {
        RegionInfo@ regionInfo = findRegion(this.workBufferRegions, regionNames[i]);
        if (@regionInfo != null)
        {
            //game.log("DBG restoreRegionFoldStates() ~ region '"+regionNames[i]+"': isFolded="+regionInfo.isFolded+" (backup: "+regionInfo.isFoldedBackup+")");
            if (regionInfo.isFolded && !regionInfo.isFoldedBackup)
            {
                this.unFoldRegionInternal(regionNames[i]);
            }
            else if (!regionInfo.isFolded && regionInfo.isFoldedBackup)
            {
                this.foldRegionInternal(regionNames[i]);
            }
            regionInfo.isFolded = regionInfo.isFoldedBackup;
        }
    }

    // Determine which messages are in folded-off regions - important for correct drawing
}
}

// Originally global, now instantiated per-tab; some logic was left over.
class ExceptionsPanel
{
    ScriptEditorTab@ parentEditorTab;
    dictionary exception_stats_nid; // NID-> dictionary{ fullDesc -> num occurences }
    array<int> nids; // Recorded NIDs - since each buffer has it's own panel instance now, there will never be more than 1
    int numLinesDrawn = 0;
    // very important to keep the dict clean after requesting script unload
    //  - even after notification more exceptions can arrive
    bool ePanelEnabled = true;

    ExceptionsPanel(ScriptEditorTab@ editorTab)
    {
        @parentEditorTab = @editorTab;
    }

    float getHeightToReserve()
    {
        if (exception_stats_nid.isEmpty()) { return 0; }
        else { return numLinesDrawn*ImGui::GetTextLineHeight() + 15; }
    }

    void onEventExceptionCaught(int nid, string arg5_from, string arg6_type, string arg7_msg)
    {
        if (parentEditorTab.currentScriptUnitID != nid)
        {
            /*game.log("DBG onEventExceptionCaught(nid="+nid+") IGNORING; (currentScriptUnitID="+parentEditorTab.currentScriptUnitID+" bufferName="+parentEditorTab.bufferName);*/
            return;
        }

        // format: FROM >> MSG (TYPE)
        string desc=arg5_from+' --> '+arg7_msg +' ('+arg6_type+')';
        this.addExceptionInternal(nid, desc);
    }

    void onEventAngelscriptExceptionCallback(int nid, int arg3_linenum, string arg5_from, string arg6_msg)
    {
        if (parentEditorTab.currentScriptUnitID != nid)
        {
            /*game.log("DBG onEventAngelscriptExceptionCallback(nid="+nid+") IGNORING; (currentScriptUnitID="+parentEditorTab.currentScriptUnitID+" bufferName="+parentEditorTab.bufferName);*/
            return;
        }

        // format: FROM >> MSG (line: LINENUM)
        string desc = arg5_from+"() --> "+arg6_msg+" (line: "+arg3_linenum+")";
        this.addExceptionInternal(nid, desc);
    }

    void addExceptionInternal(int nid, string desc)
    {
        if (!ePanelEnabled)
        return;

        // locate the dictionary for this script (by NID)
        dictionary@ exception_stats = cast<dictionary@>(exception_stats_nid[''+nid]);
        if (@exception_stats == null)
        {
            exception_stats_nid[''+nid] = dictionary();
            @exception_stats = cast<dictionary@>(exception_stats_nid[''+nid]);
            this.nids.insertLast(nid);
        }
        exception_stats[desc] = int(exception_stats[desc])+1;
    }

    void drawExceptions()
    {
        // Requests for editing entries - cannot be done while iterating!
        bool requestClearAll = false;
        dictionary@ requestClearEntryFromStats;
        string requestClearEntryKey;

        numLinesDrawn=0;
        int numEntriesDrawn=0;
        for (uint i=0; i<nids.length(); i++)
        {
            ImGui::PushID(i);

            dictionary@ info = game.getScriptDetails(nids[i]);
            dictionary@ exception_stats = cast<dictionary@>(exception_stats_nid[''+nids[i]]);

        if (@exception_stats == null) { continue; } // <----- continue, nothing to draw


            ImGui::TextDisabled('NID '+nids[i]+' ');
            if (@info != null)
            {
                ImGui::SameLine(); ImGui::Text(string(info['scriptName']));
            }
            else
            {
                ImGui::SameLine(); ImGui::TextDisabled("(NOT RUNNING)");
            }
            numLinesDrawn++;

            array<string>@tag_keys = exception_stats.getKeys();
            ImGui::SameLine(); ImGui::Text(">>");
            ImGui::SameLine(); ImGui::TextColored(  color(1,0.1, 0.2, 1), tag_keys.length() + ' exception type(s) encountered:');
            string clearall_btn_text = "Clear all";
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - (ImGui::CalcTextSize(clearall_btn_text).x + 8.f));
            if (ImGui::SmallButton(clearall_btn_text))
            {
                requestClearAll = true;
            }
            for (uint j=0; j < tag_keys.length(); j++)
            {
                ImGui::PushID(j);

                string descr = tag_keys[j];
                int tagcount = int(exception_stats[descr]);
                if (ImGui::SmallButton("Clear"))
                {
                    /*game.log("DBG epanel requested clearing '"+descr+"' - button pressed");*/
                    requestClearEntryKey = descr;
                    @requestClearEntryFromStats = @exception_stats;
                }
                ImGui::SameLine(); ImGui::TextDisabled('['+tagcount+']');
                ImGui::SameLine(); ImGui::Text(descr);

                numLinesDrawn++;
                numEntriesDrawn++;

                ImGui::PopID(); // j
            } // for (tag_keys)

            ImGui::PopID(); // i
        } // for (nids)

        // Requests for editing entries - cannot be done while iterating!
        if (requestClearEntryKey != "" && @requestClearEntryFromStats != null)
        {
            /*game.log("DBG epanel requested clearing '"+requestClearEntryKey+"' - deleting from dict");*/
            if (!requestClearEntryFromStats.delete(requestClearEntryKey))
            {
                game.log("script_editor INTERNAL ERROR: requested clearing exception '"+requestClearEntryKey+"' failed - not found");
            }
            if (numEntriesDrawn == 1)
            {
                requestClearAll=true; // We just deleted the only entry -> close panel
            }
        }

        if (requestClearAll)
        {
            /*game.log("DBG @@@ handling requestClearAll for buffer='"+parentEditorTab.bufferName+"' !!!");*/
            this.clearExceptions();
        }

    } // drawExceptions()

    void clearExceptions()
    {
        /*game.log("DBG @@@ clearExceptions() called for buffer='"+parentEditorTab.bufferName+"' !!!");*/
        this.exception_stats_nid.deleteAll();
        this.numLinesDrawn=0;
        this.nids = array<int>(); // - since each buffer has it's own panel instance now, there will never be more than 1
    }

} // class ExceptionsPanel

// Code folding with `#region`/`#endregion`
class RegionInfo
{
    int regionLineCount;
    int regionBodyStartOffset;
    int regionBodyNumChars;
    int regionStartsAtLineIndex;

    bool isFolded;
    bool isOrphan; // Not currently present in document, still checked for uniqueness. If re-entered as empty (no lines), can be unFolded. If conflicting, user gets notified by '{ }' invalid state marker.
    string foldedOffText;
    bool isFoldedBackup; // For temporary unfolding all during saving/indenting/running the buffer; use `backUpRegionFoldStates()` and `restoreRegionFoldStates()`.
}

/// Represents a pre-defined group of scripts to be displayed in menus.
class ScriptIndexerRecord
{
    // DATA:
    array<dictionary>@ fileinfos = null; // findResourceFileInfo() + added field 'scriptInfo'
    uint fileinfos_analyzed = 0;
    string rgname;
    string pattern;

    // FUNCS:

    void scanResourceGroup(string rgname, string pattern = '*')
    {
        @fileinfos = game.findResourceFileInfo(rgname, pattern);
        this.rgname = rgname;
        this.pattern = pattern;
        fileinfos_analyzed = 0;
        //  analysis is done frame-by-frame in `advanceScriptAnalysis()`
    }

    scriptinfo_utils::ScriptInfo@ getScriptInfo(uint index)
    {
        if (index < fileinfos.length() && fileinfos[index].exists('scriptInfo'))
        {
            return cast<scriptinfo_utils::ScriptInfo>(fileinfos[index]['scriptInfo']);
        }
        return null;
    }

    // Returns TRUE if there was a script pending analysis.
    bool advanceScriptAnalysis()
    {
        if (fileinfos_analyzed < fileinfos.length())
        {
            this.analyzeSingleScript(fileinfos_analyzed);
            fileinfos_analyzed++;
            return true;
        }
        return false;
    }

    void analyzeSingleScript(int index)
    {
        string filename = string(fileinfos[index]['filename']);
        string body = game.loadTextResourceAsString(filename, rgname);
        scriptinfo_utils::ScriptInfo@ scriptinfo = scriptinfo_utils::ExtractScriptInfo(body);
        /*game.log("DBG analyzeSingleScript("+index+"): File="+filename+'/RgName='+rgname
			+": title="+scriptinfo.title+", brief="+scriptinfo.brief+", text="+scriptinfo.text);*/
        fileinfos[index]['scriptInfo']  = scriptinfo;

    }
} // class ScriptIndexerRecord

// Helpers
bool isChar(uint c, string s)
{
    return s.length() > 0 && c == s[0];
}

bool isCharBlank(uint c)
{
    return isChar(c, " ") || isChar(c, "\t");
}

string trimLeft(string s)
{
    for (uint i = 0; i < s.length(); i++)
    {
        if (!isCharBlank(s[i]))
			return s.substr(i, s.length() - i);
    }
    return "";
}

RegionInfo@ findRegion(dictionary@ regionDict, string name) // Helper which checks first (not inserting NULL entry)
{
    if (regionDict.exists(name))
        return cast<RegionInfo>(regionDict[name]); 
    else
        return null;
}

// Tutorial scripts
// -----------------
// using 'heredoc' syntax; see https://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_strings.html

const string TUT_SCRIPT =
"""
// TUTORIAL SCRIPT - Shows the basics, step by step:
// * How to open UI window and handle [X] close button.
// * how to store data and update/draw them every frame.
// * How to do code folding with '#[end]region'
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// total time in seconds
float tt = 0.f;

void main()
{
    // Uncomment to close window without asking.
    //closeBtnHandler.cfgCloseImmediatelly = true;
}

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // Begin drawing window
    if (ImGui::Begin("Tutorial script", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();

        // accumulate time
        tt += dt;

        //#region format the output
        string ttStr = "Total time: " + formatFloat(tt, "", 5, 2) + "sec";
        string dtStr = "Delta time: " + formatFloat(dt, "", 7, 4) + "sec";
        //#endregion

        //#region render the output
        ImGui::Text(ttStr);
        ImGui::SameLine();
        ImGui::Text(dtStr);
        //#endregion

        // End drawing window
        ImGui::End();
    }
}
""";
