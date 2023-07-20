// Project Rigs of Rods (http://www.rigsofrods.org)
//              ** SCRIPT EDITOR **
// Documentation: http://developer.rigsofrods.org
// ================================================

ScriptEditor editor;

// Callback functions for the game:
// --------------------------------

void main() // Invoked by the game on startup
{
    game.log("Script editor started!");
    game.registerForEvent(SE_ANGELSCRIPT_MSGCALLBACK);
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
    string BUFFER_TEXTINPUT_LABEL = "bufferTextInputLbl";
    bool drawLineNumbers=true;
    color lineNumberColor=color(1.f,1.f,0.7f,1.f);
    bool drawLineLengths=true;
    color lineLenColor=color(0.4f,0.3f,0.2f,1.f);
    
    string buffer;
    array<dictionary> bufferLinesMeta;
    
    array<dictionary> messages;

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
            if (ImGui::Button("|> RUN"))
            {
                this.runBuffer();
            }
            
            ImGui::EndMenuBar();
        }
    }
    
    protected void drawTextArea()
    {
        const float NUMBERSCOLUMN_WIDTH = 25;
        const float LINELENCOLUMN_WIDTH = 36;
        const float MENUBAR_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
        const float FOOTER_HEIGHT = ImGui::GetTextLineHeightWithSpacing() + 8; // +2x vertical spacing
    
        vector2 textAreaSize(ImGui::GetWindowSize().x - (20+NUMBERSCOLUMN_WIDTH), ImGui::GetWindowSize().y - (MENUBAR_HEIGHT + FOOTER_HEIGHT));
        vector2 screenCursor = ImGui::GetCursorScreenPos();
        if (this.drawLineNumbers)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NUMBERSCOLUMN_WIDTH);
        if (this.drawLineLengths)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LINELENCOLUMN_WIDTH);            
        bool changed = ImGui::InputTextMultiline(BUFFER_TEXTINPUT_LABEL, this.buffer, textAreaSize);
        if (changed)
        {
            this.analyzeBuffer();
        }
        
        // Trick: re-enter the inputtext panel to scroll it; see https://github.com/ocornut/imgui/issues/1523
        float scrollX, scrollY, maxScrollX, maxScrollY;
        ImGui::BeginChild(ImGui::GetID(BUFFER_TEXTINPUT_LABEL));
        scrollX = ImGui::GetScrollX();
        scrollY = ImGui::GetScrollY();
        maxScrollX = ImGui::GetScrollMaxX();
        maxScrollY = ImGui::GetScrollMaxY();
        ImGui::EndChild();
        
        // Line numbers (and lengths)
        vector2 linenumCursorBase = screenCursor+vector2(0.f, -scrollY) + /*frame padding:*/vector2(0.f, 4.f);
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();
        int bufferNumLines = int(this.bufferLinesMeta.length());
        if (this.drawLineNumbers || this.drawLineLengths)
        {
            
            
            vector2 linenumCursor = linenumCursorBase;
            
            for (int linenum = 1; linenum <= bufferNumLines; linenum++)
            {
                if (this.drawLineNumbers)
                {
                    drawlist.AddText(linenumCursor, this.lineNumberColor, ""+linenum);
                    linenumCursor.x += NUMBERSCOLUMN_WIDTH;
                }
                
                if (this.drawLineNumbers)
                {
                    drawlist.AddText(linenumCursor, this.lineLenColor,
                        ""+int(this.bufferLinesMeta[linenum-1]['len']));
                    linenumCursor.x += LINELENCOLUMN_WIDTH;                    
                }
                
                // prepare new Line
                linenumCursor.x = linenumCursorBase.x;
                linenumCursor.y += ImGui::GetTextLineHeight();
            }
        }
        
        // Test: messages
        int lastTextRow = -1;
        float lastTextRowCursorX = 0.f;
        for (uint i = 0; i < this.messages.length(); i++)
        {
            int msgRow = int(this.messages[i]['row']);
            int msgType = int(this.messages[i]['type']);
            string msgText = string(this.messages[i]['message']);
            if (msgType == 0) // "expected... instead found"
            {
                // Draw line marker on the left
                vector2 msgCursor = linenumCursorBase + vector2(0.f, ImGui::GetTextLineHeight()*(msgRow-1));
                drawlist.AddText(msgCursor, color(1.f,0.5f,0.4f,1.f), "  !!");
                
                // Draw message text on the right; accumulate cursor offset
                float textCursorX = lastTextRowCursorX;
                if (msgRow != lastTextRow)
                {
                    // FIXME: calculate actual line length!
                    textCursorX = 100;
                }
                drawlist.AddText(msgCursor + vector2(textCursorX, 0.f), color(0.7f,0.7f,1.0f,1.f), msgText);
                lastTextRowCursorX = textCursorX + ImGui::CalcTextSize(msgText).x;
                lastTextRow = msgRow;
            }
        }
        
        // footer with status info
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.f);
        ImGui::Separator();
        ImGui::Text("lines: "+bufferNumLines+"; messages:"+this.messages.length()+" scroll: X="+scrollX+"/"+maxScrollX+"; Y="+scrollY+"/"+maxScrollY+"");
    }
    
    void runBuffer()
    {
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
    }
    
    void analyzeBuffer()
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
}
