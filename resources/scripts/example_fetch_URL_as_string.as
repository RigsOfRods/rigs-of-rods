// == Project Rigs of Rods (http://www.rigsofrods.org) ==
// EXAMPLE SCRIPT - `game.fetchUrlAsStringAsync()`
// ======================================================

// Context:
string payload = "";
string statusmsg = "";
int statustype = 0;
int statuscount_progress=0;
int statuscount_success=0;
int statuscount_failure=0;
int httpcode = 0;
string url = "https://developer.rigsofrods.org/modules.html"; // plain http gives "301 moved permanently"
bool running = false;
bool done = false;

// Startup:
void main()
{
     game.registerForEvent(SE_ANGELSCRIPT_THREAD_STATUS);
}

// Event handling:
void eventCallbackEx(scriptEvents ev, 
    int arg1, int arg2, int arg3, int arg4, 
    string arg5, string arg6, string arg7, string arg8)
{
    if (ev == SE_ANGELSCRIPT_THREAD_STATUS)
    {
        statustype = arg1;
        statusmsg = arg5;    
    
        switch (arg1){
            case ASTHREADSTATUS_CURLSTRING_PROGRESS:
                statuscount_progress++;
                break;

            case ASTHREADSTATUS_CURLSTRING_SUCCESS: 
                httpcode = arg2;
                statuscount_success++;
                done=true;
                running=false;
                break;

            case ASTHREADSTATUS_CURLSTRING_FAILURE: 
                httpcode = arg2;
                statuscount_failure++;
                done=true;
                running=false;
                break;
        }
    }
}

// GUI drawing:
void frameStep(float dt)
{
    ImGui::Text("Running: "+running+"; Done:"+done);

     if (!running && ImGui::Button(url))
    {
         done = false;
         running = true;
         game.fetchUrlAsStringAsync(url, "ABCD");
    }
    if (running || done)
    {
        ImGui::TextDisabled("stats:");
         ImGui::Text("statuscount_progress:"+statuscount_progress);
         ImGui::Text("statuscount_success:"+statuscount_success);
         ImGui::Text("statuscount_failure:"+statuscount_failure);
         ImGui::TextDisabled("data:");
         ImGui::Text("httpcode:"+httpcode);
         ImGui::Text("statustype:"+statustype);
         ImGui::Text("statusmsg:"+statusmsg);

    }
}
