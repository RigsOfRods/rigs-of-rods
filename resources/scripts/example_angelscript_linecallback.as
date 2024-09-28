// TEST SCRIPT - angelscript linecallback (tracks executed lines)
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// GUI (intentionally at the top to have fewer lines in the output):
void frameStep(float dt)
{
    if (ImGui::Begin("Example - line callbacks", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
    
        ImGui::TextDisabled("Frames:"+framecounter+", Samples: "+totalSamples+"(per frame :"+linenumsLite.length()+')');
        ImGui::Text("[Line]  Total");
        
        for (uint i=0; i<lineHitsAggregatedLite.length(); i++) {
            float y = ImGui::GetCursorPosY();
            ImGui::TextDisabled("[ "+formatInt(i+1, "0", 2)+" ]"); 
            ImGui::SameLine(); ImGui::Text(""+formatInt(lineHitsAggregatedLite[i], '0', 4)); 
            // manual linebreak, to align with the editor window
            ImGui::SetCursorPosY(y + ImGui::GetTextLineHeight());
        }
        framecounter++;
        ImGui::End();
    }        
}

 //CONTEXT:
int lowestLinenum = 999999;
int highestLinenum = 0;
int totalSamples = 0;
array<int> linenumsLite;
array<int> lineHitsAggregatedLite;
int framecounter = 0;

// SETUP:
void main(){ 
  // this automagically attaches the LineCallback to angelscript.
  game.registerForEvent(SE_ANGELSCRIPT_LINECALLBACK);
}

// EVENT HANDLING:
void eventCallbackEx(scriptEvents ev, int a1, int a2, int a3, int a4, string a5, string a6, string a7, string a8)
{
   if (ev == SE_ANGELSCRIPT_LINECALLBACK) { addSample(a2); }
}

// HELPERS:
void addSample(int a2)
{
    if (a2 < lowestLinenum) { lowestLinenum=a2; }
    else if (a2 > highestLinenum) { highestLinenum = a2; }
    // detect repeating
    if (a2 < highestLinenum && a2 == lowestLinenum) {  aggregateLineHitsLite(); }
    linenumsLite.insertLast(a2);
    totalSamples++;
}

void aggregateLineHitsLite()
{
    // allocate
    lineHitsAggregatedLite.resize(highestLinenum+1);

    // aggregate
    for (uint i=0; i<linenumsLite.length(); i++) {
        lineHitsAggregatedLite[linenumsLite[i]] ++;
    }
    // reset
    linenumsLite.resize(0); // clear all
}