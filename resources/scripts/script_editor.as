// Project Rigs of Rods (http://www.rigsofrods.org)
//              ** SCRIPT EDITOR **
// Documentation: http://developer.rigsofrods.org
// ================================================

// Global definitions:
// -------------------

// from <angelscript.h>
enum asEMsgType
{
	asMSGTYPE_ERROR       = 0,
	asMSGTYPE_WARNING     = 1,
	asMSGTYPE_INFORMATION = 2
};
const string BUFFER_TEXTINPUT_LABEL = "##bufferTextInputLbl";
ScriptEditor editor;

// Config:
// -------

// LINE METAINFO COLUMNS
    // Line numbers
    bool drawLineNumbers=true;
    color lineNumberColor=color(1.f,1.f,0.7f,1.f);
    float lineNumColumnWidth = 25;
    // Line char counts        
    bool drawLineLengths=true;
    color lineLenColor=color(0.5f,0.4f,0.3f,1.f);
    float lineLenColumnWidth = 25;
    // Line err counts (error may span multiple AngelScript messages!)
    bool drawLineErrCounts = true;
    color lineErrCountColor = color(1.f,0.3f,0.2f,1.f);
    float errCountColumnWidth = 18;
// END line metainfo columns

// EDITOR settings
    color errorTextColor = color(1.f,0.3f,0.2f,1.f);
    color errorTextBgColor = color(0.4f,0.1f,0.3f,1.f);
    bool drawErrorText = true;
// END editor

// Callback functions for the game:
// --------------------------------

void main() // Invoked by the game on startup
{
    game.log("Script editor started!");
    game.registerForEvent(SE_ANGELSCRIPT_MSGCALLBACK);
    editor.buffer=TUT_SCRIPT;
    editor.analyzeLines();
}


void frameStep(float dt) // Invoked regularly by the game; the parameter is elapsed time in seconds.
{
    editor.draw();
}


void eventCallbackEx(scriptEvents ev, // Invoked by the game when a registered event is triggered
    int arg1, int arg2ex, int arg3ex, int arg4ex,
    string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    if (ev == SE_ANGELSCRIPT_MSGCALLBACK)
    {
        editor.onEventAngelScriptMsg(
            /*id:*/arg1, /*eType:*/arg2ex, /*row:*/arg3ex, /*col:*/arg4ex,
            /*sectionName:*/arg5ex, /*msg:*/arg6ex);
    }
}

// The script editor logic:
// ------------------------

class ScriptEditor
{
    
    // THE TEXT BUFFER
        string buffer; // buffer data
        array<dictionary> bufferLinesMeta; // metadata for lines, see `analyzeLines()`
        array<array<uint>> bufferMessageIDs;
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

    void draw()
    {
        int flags = ImGuiWindowFlags_MenuBar;
        ImGui::Begin("Script editor", /*open:*/true, flags);
        
        this.drawMenubar();
        this.drawTextArea();

        ImGui::End();
    }
    
    protected void drawMenubar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Dummy menu item")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View"))
            {
                ImGui::TextDisabled("Line info:");
                ImGui::Checkbox("Line numbers", /*inout:*/drawLineNumbers);
                ImGui::Checkbox("Char counts", /*inout:*/drawLineLengths);
                ImGui::Checkbox("Error counts", /*inout:*/drawLineErrCounts);
                ImGui::Checkbox("Error text", /*inout:*/drawErrorText);
                ImGui::EndMenu();
            }              
            if (ImGui::Button("|> RUN"))
            {
                this.runBuffer();
            }
            
            ImGui::EndMenuBar();
        }
    }
    
    protected void drawLineInfoColumns(vector2 screenCursor)
    {
        // draws columns with info (line number etc...) on the left side of the text area
        // ===============================================================================
        
        vector2 linenumCursorBase = screenCursor+vector2(0.f, -scrollY);
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();
        int bufferNumLines = int(this.bufferLinesMeta.length());
        
        vector2 linenumCursor = linenumCursorBase;
        
        for (int linenum = 1; linenum <= bufferNumLines; linenum++)
        {
            int lineIdx = linenum - 1;
            if (drawLineNumbers)
            {
                drawlist.AddText(linenumCursor, lineNumberColor, ""+linenum);
                linenumCursor.x += lineNumColumnWidth;
            }
            
            if (drawLineLengths)
            {
                drawlist.AddText(linenumCursor, lineLenColor,
                    ""+int(this.bufferLinesMeta[lineIdx]['len']));
                linenumCursor.x += lineLenColumnWidth;                    
            }
            
            // draw message count
            if (drawLineErrCounts)
            {
                
                if (bufferMessageIDs.length() > lineIdx // sanity check
                    && bufferMessageIDs[lineIdx].length() > 0)
                {
                    // Draw num errors to the column

                    drawlist.AddText(linenumCursor, color(1.f,0.5f,0.4f,1.f), ""+bufferMessageIDs[lineIdx].length());        
                }
                linenumCursor.x += errCountColumnWidth;
            } 
            
            // prepare new Line
            linenumCursor.x = linenumCursorBase.x;
            linenumCursor.y += ImGui::GetTextLineHeight();
        }
    
           
    }
    
    protected void drawTextAreaErrors(vector2 screenCursor)
    {
        // draw messages to text area
        //===========================
        
        if (!drawErrorText)
            return;
            
        vector2 msgCursorBase = screenCursor + vector2(4, -this.scrollY);
        
        int bufferNumLines = int(this.bufferLinesMeta.length());
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();
        
        for (int linenum = 1; linenum <= bufferNumLines; linenum++)
        {
            int lineIdx = linenum - 1;    
            vector2 lineCursor = msgCursorBase  + vector2(0.f, ImGui::GetTextLineHeight()*lineIdx);
            
            if (this.bufferMessageIDs.length() <= lineIdx) // sanity check
                continue;
            
            for (uint i = 0; i < this.bufferMessageIDs[lineIdx].length(); i++)
            {
                uint msgIdx = this.bufferMessageIDs[lineIdx][i];
        
                int msgRow = int(this.messages[msgIdx]['row']);
                int msgCol = int(this.messages[msgIdx]['col']);
                int msgType = int(this.messages[msgIdx]['type']);
                int msgStartOffset = int(this.messages[msgIdx]['startOffset']);
                string msgText = string(this.messages[msgIdx]['message']);
  
                // Draw message text with background
                string errText = "^ "+msgText;
                vector2 errTextSize = ImGui::CalcTextSize(errText);
                vector2 tst = ImGui::CalcTextSize("calcTextSizetest");

                vector2 textPos = lineCursor + vector2(
                    ImGui::CalcTextSize(buffer.substr(msgStartOffset, msgCol)).x, // X offset - under the indicated text position
                    ImGui::GetTextLineHeight()-2); // Y offset - slightly below the current line
                drawlist.AddRectFilled(textPos, textPos+errTextSize, errorTextBgColor);
                drawlist.AddText(textPos , errorTextColor, errText);
                
                // advance to next line
                lineCursor.y += ImGui::GetTextLineHeight();
            }
        }            
            

    }
    
    protected void drawTextArea()
    {
        
        // Reserve space for the metainfo columns
        float metaColumnsTotalWidth = 0.f;
        if (drawLineNumbers)
            metaColumnsTotalWidth += lineNumColumnWidth;
        if (drawLineLengths)
            metaColumnsTotalWidth += lineLenColumnWidth;
        errCountOffset = metaColumnsTotalWidth;
        if (drawLineErrCounts)
            metaColumnsTotalWidth += errCountColumnWidth;
           
        // Draw the multiline text input
        vector2 textAreaSize(
            ImGui::GetWindowSize().x - (metaColumnsTotalWidth + 20), 
            ImGui::GetWindowSize().y - 90);
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
        }        
        
        this.drawLineInfoColumns(screenCursor);
        screenCursor.x+= metaColumnsTotalWidth;
        this.drawTextAreaErrors(screenCursor);
        
        
        // footer with status info
        
        ImGui::Separator();
        ImGui::Text("lines: "+bufferLinesMeta.length()
            +"; messages:"+this.messages.length()
                +"(err:"+this.messagesTotalErr
                +"/warn:"+this.messagesTotalWarn
                +"/info:"+this.messagesTotalInfo
            +") scroll: X="+scrollX+"/"+scrollMaxX
                +"; Y="+scrollY+"/"+scrollMaxY+"");
    }
    
    void runBuffer()
    {
        this.bufferMessageIDs.resize(0); // clear all
        this.messages.resize(0); // clear all
        
        
        game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED, {
            {'filename', '(editor buffer)'}, // Because we supply the buffer, this will serve only as display name
            {'buffer', this.buffer },
            {'category', SCRIPT_CATEGORY_CUSTOM}
        });
    }
    
    void onEventAngelScriptMsg(int scriptUnitId, int msgType, int row, int col, string sectionName, string message)
    {
        /*game.log("DBG onEventAngelScriptMsg(): scriptUnitId:" + scriptUnitId
            + ", msgType:" + msgType + ", row:" + row + ", col:" + col // ints
            + ", sectionName:" + sectionName + ", message:" + message); // strings*/
            
        messages.insertLast({ {'type', msgType}, {'row',row}, {'col', col}, {'sectionName', sectionName}, {'message', message} });
        this.analyzeMessages();
    }
    
    void analyzeLines()
    {
        this.analyzeBuffer();
        this.analyzeMessages();
    }
    
    private void analyzeBuffer() // helper for `analyzeLines()`
    {
        this.bufferLinesMeta.resize(0); // clear all
        int startOffset = 0;
        this.bufferLinesMeta.insertLast({ {'startOffset', startOffset} });
        int doubleslashCommentStart = -1; // offset, includes leading "//"; -1 means None
        int doubleslashNumSlashes = 0;
        
        for (uint i = 0; i < this.buffer.length(); i++)
        {
            if (this.buffer[i] == uint(0x2f)) // ASCII forward slash '/'
            {
                if (doubleslashCommentStart==-1 && doubleslashNumSlashes == 0)
                {
                    // no comment yet and found '/' - mark it.
                    doubleslashCommentStart = i;
                }
                doubleslashNumSlashes++;
            }
            else
            {
                doubleslashNumSlashes = 0;
            }
        
            if (this.buffer[i] == uint(0x0a)) // ASCII newline
            {
                // Finish line
                uint lineIdx = this.bufferLinesMeta.length() - 1;
                int endOffset = i;
                this.bufferLinesMeta[lineIdx]['endOffset'] = endOffset;
                int len = endOffset - startOffset;
                this.bufferLinesMeta[lineIdx]['len'] = len;
                this.bufferLinesMeta[lineIdx]['doubleslashCommentStart'] = doubleslashCommentStart;
                int nonCommentLen = (doubleslashCommentStart >= 0) ? doubleslashCommentStart - startOffset : len;
                this.bufferLinesMeta[lineIdx]['nonCommentLen'] = nonCommentLen;
                
                // Start new line
                startOffset = i+1;
                this.bufferLinesMeta.insertLast({ {'startOffset', startOffset} });
                doubleslashCommentStart = -1; // none yet.
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
            int msgRow = int(this.messages[i]['row']);
            int msgType = int(this.messages[i]['type']);
            string msgText = string(this.messages[i]['message']);
            int lineIdx = msgRow-1;
            
            
            
            
            // update line stats
            if (msgType == asMSGTYPE_ERROR)
            {
                this.bufferLinesMeta[lineIdx]['messagesNumErr'] = int(this.bufferLinesMeta[lineIdx]['messagesNumErr'])+1;
                this.bufferMessageIDs[lineIdx].insertLast(i);
                this.messagesTotalErr++;
            }
            if (msgType == asMSGTYPE_WARNING)
            {
                this.bufferLinesMeta[lineIdx]['messagesNumWarn'] = int(this.bufferLinesMeta[lineIdx]['messagesNumWarn'])+1;
                this.messagesTotalWarn++;
            }
            if (msgType == asMSGTYPE_INFORMATION)
            {
                this.bufferLinesMeta[lineIdx]['messagesNumInfo'] = int(this.bufferLinesMeta[lineIdx]['messagesNumInfo'])+1;
                this.messagesTotalInfo++;
            }
        }
        
    }
 
}

// Tutorial scripts 
// -----------------
// using 'heredoc' syntax; see https://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_strings.html

const string TUT_SCRIPT =
"""
// TUTORIAL SCRIPT - Shows the basics, step by step:
// how to store data and update/draw them every frame.
// ===================================================

// total time in seconds
float tt = 0.f;

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    // accumulate time
    tt += dt;
    
    // format the output 
    string ttStr = "Total time: " + formatFloat(tt, "", 5, 2) + "sec";
    string dtStr = "Delta time: " + formatFloat(dt, "", 5, 2) + "sec";
    
    // draw the output to implicit window titled "Debug"
    ImGui::Text(ttStr + "; " + dtStr);
}
""";
