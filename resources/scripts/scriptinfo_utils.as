/// \title SCRIPT INFO PARSER
/// \brief Extracts info from '///' doc comments in .as files.
/// Recognized commands are `\title`, `\brief`.
//  Doc comments must start as the first thing on top of the file.
/// Usage:
///   `#include <scriptinfo_utils.as>` ~~ adds the utils to your script
///   `ScriptInfo sinfo = ExtractScriptInfo(scriptstring);` ~~ Processes the script string
///   `game.log("<ScriptInfo> title:"+sinfo.title+", brief:"+sinfo.brief+", text:"+sinfo.text);`
// ======================================================

class ScriptInfo
{
   string title;
   string brief;
   string text;
}

ScriptInfo@ ExtractScriptInfo(string body)
{
   ScriptInfoHelper h;
    // find the annotations
    ScriptInfo scriptinfo;
    int state = 0; //-1 DONE, 0 = start of line, 1/2/3=number of slashes at start of line, 4=blanks after slashes,
                           //5=capturing cmd, 6=capturing title, 7=cabturing subtitle, 8=capturing long description
    uint pos = 0;
    uint capture_startpos = 0;

    while (state != -1 && pos < body.length())
    {
      h.c = body[pos];
      game.log("DBG scriptinfo: state="+state+", capture_startpos="+capture_startpos+", c="+body.substr(pos, 1));
      switch (state) {
       case 0: // line start 
       case 1: //  /
       case 2: //  // ...
        if (h.ischar('/')){ state++; } else {state=-1;} 
        break;
       case 3: 
          if ( h.isblank() ) { state=4;  } 
          else if (h.ischar('\\')) { capture_startpos = pos; state=5;  }
          else { capture_startpos = pos; state=8; }
         break;
       case 4:  // blanks after slashes
        if (!h.isblank()) {    if (h.ischar('\\')) { capture_startpos = pos; state=5; }  else { capture_startpos = pos; state=8; } }
         else if (h.isEol()) {  state=0;  }
         break;
       case 5: // cmd, like '\file'
          if (h.isblank()) { 
            string cmd = body.substr(capture_startpos, pos-capture_startpos); game.log('cmd: "'+cmd+'"'); 
            if (h.isEol()) {state=0;} else if (cmd=='\\title') { state=6; capture_startpos = pos; } else if (cmd=='\\brief') {state=7;capture_startpos = pos; } else {state=8;capture_startpos = pos; }  }
           break;
       case 6:
         if (h.isEol()) { state=0; string cmd = body.substr(capture_startpos, pos-capture_startpos); ; scriptinfo.title=h.trimStart(cmd); }
         break;
       case 7:
          if (h.isEol()) {state=0;string cmd = body.substr(capture_startpos, pos-capture_startpos);  scriptinfo.brief=h.trimStart(cmd);  }
         break;
       case 8:
         if (h.isEol()) {state=0; string cmd = body.substr(capture_startpos, pos-capture_startpos);  scriptinfo.text+=cmd+' '; }
         break;
         
      }
      pos++;
    }
    return scriptinfo;
}

class ScriptInfoHelper
{
  uint c;
  bool ischar(string s) { for (uint i=0; i<s.length(); i++) { if (c==s[i]) return true; } return false; }
  bool isblank() { return ischar( '\n\t '); }  
  bool isEol() { return ischar( '\n'); } 
  string trimStart(string s) { 
    int found=s.findFirstNotOf('\t '); 
    game.log("trimStart(): s='"+s+"', found:"+found+", dryrun: '"+s.substr(found)+"'");
    if (found > 0) {   return s.substr(found);}
    else{  return s;} }
}

/*
// ###### testing #######

string scriptstring = '/// \\title DemoInfo\n///   \\brief    DemoBrief\n/// Lorem...\n///   Ipsum!\n';

void main() {
  ScriptInfo sinfo = ExtractScriptInfo(scriptstring);// ~~ Processes the script string
  game.log("<ScriptInfo> title:"+sinfo.title+", brief:"+sinfo.brief+", text:"+sinfo.text);
}

*/
