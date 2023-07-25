#include "imgui_utils.as"

// Constants
string SLA = "/";

// Context:
string urlbase = "https://developer.rigsofrods.org";
string url = "/d4/d07/group___script2_game.html"; 
array<string> result_hint = {'RDY', 'netOK', 'someH2', 'ourCLASSES', 'DONE', 'aURL', 'aNAME', 'ourH2' 'netFAIL'};
int result = 0;  //RDY
string data = "";
uint data_offset = 0;  // for seeking
string current_tag = "";
string current_url = "";
dictionary results; // ClassName->URL pairs
 // stats:
int tags_processed = 0;
dictionary tag_stats;

// Startup:
void main()
{ 
    game.registerForEvent(SE_ANGELSCRIPT_THREAD_STATUS);
    game.fetchUrlAsStringAsync(urlbase+url, "game object doc");
}

// Event handling:
void eventCallbackEx(scriptEvents ev, 
    int arg1, int arg2, int arg3, int arg4, 
    string arg5, string arg6, string arg7, string arg8)
{
    if (ev == SE_ANGELSCRIPT_THREAD_STATUS) {
        switch (arg1){
            case ASTHREADSTATUS_CURLSTRING_SUCCESS:  result=1; data=arg5; break;
            case ASTHREADSTATUS_CURLSTRING_FAILURE: result = 8; break;
        }
    }
}

// GUI drawing 
void frameStep(float dt)
{
    ImGui::Text("Status: "+result_hint[result] + " (position: "+data_offset+"/"+data.length()+", cur tag: '"+current_tag+"')");

   array<string>@res_keys = results.getKeys();
for (uint i=0; i < res_keys.length(); i++) {ImGui::Bullet(); ImGui::SameLine(); ImHyperlink(string(results[res_keys[i]]), res_keys[i]); }

ImGui::Separator();
   array<string>@tag_keys = tag_stats.getKeys();
    ImGui::Text("Stats: total tags:"+tags_processed+", total types:"+tag_keys.length()+" (only showing 2+ hits)");
   for (uint i=0; i < tag_keys.length(); i++) { 
      int tagcount = int(tag_stats[tag_keys[i]]);
      if (tagcount>1) { ImGui::Bullet(); ImGui::SameLine(); ImGui::Text(tag_keys[i]+": "+tagcount); }
   }
  ImGui::Separator();
   html_parse_next();  
}

void html_parse_next()
{
    if (result == 1) { html_seekstarttag(); }
    if (result == 1 && (current_tag == "H2" || current_tag == "h2")) { result = 2; }  
    if (result == 2) { html_seekchar(">"); }
    if (result == 2) { html_forward(1); }
    if (result == 2) { if (html_matchstring('<a name="nested-classes"></a>\n')){result=7;} }
    if (result == 7) { if (html_matchstring( "Data Structures")) {result=3; } else {result = 1;} }
    if (result == 3) { html_seekstarttag(); }
    if (result == 3) { if (html_matchstring(' class="el" href="')) {result=5; } }
   int URLstart = data_offset;
   if (result == 5) {html_seekchar('"'); }
   if (result == 5) { current_url = urlbase + data.substr(URLstart, data_offset-URLstart).substr(5); }
   if (result == 5) { if (html_matchstring('">')) { result=6;} }
   int Cstart = data_offset;
    if (result == 6) { html_seekchar("<"); }
    if (result == 6) { results.set(data.substr(Cstart, data_offset-Cstart), current_url); result=3; current_url=""; }
    if (result == 3 && (current_tag == "H2" || current_tag == "h2")) { result = 4; }  
}

// HTML parsing helpers
void html_forward(uint count)
{
      data_offset+=count;
       if ( data.length() <= data_offset) result=4; // sanity
}

void html_seekstarttag()
{
    if (result != 4) { html_seekchar("<"); }
    html_forward(1);
    int tagstart = data_offset;
    if (data[data_offset] == SLA[0]) { current_tag = ""; return; } // filter out end tags!
    if (result != 4) { html_seekchar("> \n\t"); }
   if (result != 4) { 
      current_tag = data.substr(tagstart, data_offset-tagstart); 
      tags_processed++;
      tag_stats[current_tag]=int( tag_stats[current_tag])+1;
   }
}

void html_seekchar(string strc)
{
       while(strc.findFirst(data.substr(data_offset,1)) < 0) { html_forward(1);} 
}

bool html_matchstring(string str)
{
  string su= data.substr(data_offset, str.length());
  game.log('M: '''+str+"' vs '"+su);
  if (data.length() > data_offset+str.length() && su==str) {html_forward(str.length()); return true; }
  return false;
}
