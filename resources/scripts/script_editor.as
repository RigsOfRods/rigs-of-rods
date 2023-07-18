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
    void draw()
    {
        int flags = ImGuiWindowFlags_MenuBar;
        ImGui::Begin("Script editor", /*open:*/true, flags);
        
        this.drawMenubar();
        this.drawTextArea();

        ImGui::End();
    }
    
    void drawMenubar()
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
    
    void drawTextArea()
    {
        const float NUMBERSCOLUMN_WIDTH = 10;
    
        vector2 textAreaSize(ImGui::GetWindowSize().x - (20+NUMBERSCOLUMN_WIDTH), ImGui::GetWindowSize().y - 50);
        ImGui::SetNextItemWidth(textAreaSize.x);
        // FIXME: currently we can't set height because the binding is `InputTextMultiline(const string&in, string&inout)` and thats it.
        vector2 screenCursor = ImGui::GetCursorScreenPos();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + NUMBERSCOLUMN_WIDTH);
        bool changed = ImGui::InputTextMultiline(BUFFER_TEXTINPUT_LABEL, this.buffer);
        if (changed)
        {
            this.bufferNumLines = 1;
            for (uint i = 0; i < this.buffer.length(); i++)
            {
                if (this.buffer[i] == uint(0x0a)) // ASCII newline
                {
                    this.bufferNumLines++;
                }
            }
        }
        
        // Trick: re-enter the inputtext panel to scroll it; see https://github.com/ocornut/imgui/issues/1523
        float scrollX, scrollY, maxScrollX, maxScrollY;
        ImGui::BeginChild(ImGui::GetID(BUFFER_TEXTINPUT_LABEL));
        scrollX = ImGui::GetScrollX();
        scrollY = ImGui::GetScrollY();
        maxScrollX = ImGui::GetScrollMaxX();
        maxScrollY = ImGui::GetScrollMaxY();
        ImGui::EndChild();
        
        // Test: line numbers
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();
        vector2 linenumCursorBase = screenCursor+vector2(0.f, -scrollY) + /*frame padding:*/vector2(0.f, 4.f);
        vector2 linenumCursor = linenumCursorBase;
        for (int linenum = 1; linenum <= this.bufferNumLines; linenum++)
        {
            drawlist.AddText(linenumCursor, color(1.f,1.f,0.7f,1.f), ""+linenum);
            linenumCursor.y += ImGui::GetTextLineHeight();
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
        ImGui::Separator();
        ImGui::Text("lines: "+this.bufferNumLines+"; messages:"+this.messages.length()+" scroll: X="+scrollX+"/"+maxScrollX+"; Y="+scrollY+"/"+maxScrollY+"");
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
    
    string buffer;
    int bufferNumLines = 1;
    string BUFFER_TEXTINPUT_LABEL = "bufferTextInputLbl";
    array<dictionary> messages;
}
