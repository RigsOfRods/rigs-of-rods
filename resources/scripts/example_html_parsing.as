// HTML PARSER PROTOTYPE 3
// =====================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// DEFS:
enum Event { NONE, TEXT, ELEM_OPEN, ELEM_CLOSE, ATTR_NAME, ATTR_VAL }
array<string> EventNames = { 'NONE', 'TEXT', 'ELEM_OPEN', 'ELEM_CLOSE', 'ATTR_NAME', 'ATTR_VAL' };
enum State {EOF, TEXTNODE, 
    OPENBRACE, CLOSEBRACE, TAGSLASH,
    OPENTAGNAME, CLOSETAGNAME, OPENTAGWHITE, CLOSETAGWHITE,
    ATTRNAME, ATTREQUAL, ATTRVAL}
funcdef void EventHandlerFunc(Event, string);

// VARS:
EventHandlerFunc@ handler;
State state = TEXTNODE;
uint valStart = 0;
string data;
uint i;

// FUNCS:
bool ischar(uint c, string s) { for (uint i=0; i<s.length(); i++) { if (c==s[i]) return true; } return false; }
bool isblank(uint c) { return ischar(c, '\n\t '); }
void runhandler(Event e) {if (e==TEXT){ runhandlertext(); }else{ runhandlersubstr(e); }}
void runhandlersubstr(Event e) { handler(e, data.substr(valStart, i-valStart)); }
void runhandlertext() { bool blanks=true; for (uint j=valStart; j<i; j++){blanks= blanks && isblank(data[j]);} if (!blanks) {runhandlersubstr(TEXT);} }

// PARSING:
void parse()
{
  for (i = 0; i < data.length(); i++)
  {
     uint c = data[i];
     switch (state)
     {
       case TEXTNODE: if (ischar(c, '<')){state=OPENBRACE; runhandler(TEXT);} break;
       case OPENBRACE: if (!isblank(c)) { if (!ischar(c, '/')){ state=OPENTAGNAME; valStart=i; }else{ state=TAGSLASH; }  } break;
       case CLOSEBRACE: if (ischar(c, '<')){state=OPENBRACE;} else {state=TEXTNODE; valStart=i;} break;
       case TAGSLASH: if (!isblank(c)){ state=CLOSETAGNAME; valStart=i; } break;
       case OPENTAGNAME: if (ischar(c, '>')){state=CLOSEBRACE; runhandler(ELEM_OPEN);} else if (isblank(c)) { state=OPENTAGWHITE; runhandler(ELEM_OPEN); } break;
       case CLOSETAGNAME:  if (ischar(c, '>')){state=CLOSEBRACE; runhandler(ELEM_CLOSE);}  if (isblank(c)) { state=CLOSETAGWHITE; runhandler(ELEM_CLOSE); } break;
       case CLOSETAGWHITE: if (ischar(c, '>')) {state=TEXTNODE;} break;
       case OPENTAGWHITE: if (!isblank(c)){state=ATTRNAME; valStart=i;} break;
       case ATTRNAME: if (isblank(c)){state=ATTREQUAL; runhandler(ATTR_NAME);} break;
       case ATTREQUAL: if (ischar(c, '=')) {state=ATTRVAL;} break;
       case ATTRVAL: if (isblank(c)) { state=OPENTAGWHITE; runhandler(ATTR_VAL); } break;
     }
  }
}

// =========== testing ==========

// var:
array<Event> testEvents;
array<string> testVals;
// conf:
void testHandler(Event e, string value) { testEvents.insertLast(e); testVals.insertLast(value); }

void main()
{
    @handler = testHandler;
    data = '<html> <head> <title>Sample data</title> </head> <body> AngelScript rocks! </body> </html>';
    parse();
}

void frameStep(float dt)
{
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
    
        ImGui::TextDisabled("Html parser 3");
        for (uint i=0; i < testEvents.length(); i++) { ImGui::Text('['+i+'] event: '+EventNames[testEvents[i]]+", value: " + testVals[i]); }
            
        ImGui::End();
    }


}