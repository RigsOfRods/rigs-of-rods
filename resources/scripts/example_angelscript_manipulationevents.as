// \title Script manipulation events
// \brief Demo of ASMANIP_* events
// ===================================================

string ManipEventToString(int asmanip)
{
    if (asmanip == ASMANIP_CONSOLE_SNIPPET_EXECUTED)
        return "ASMANIP_CONSOLE_SNIPPET_EXECUTED";
    else if (asmanip == ASMANIP_SCRIPT_LOADED)
        return "ASMANIP_SCRIPT_LOADED";
    else if (asmanip == ASMANIP_SCRIPT_UNLOADING)
        return "ASMANIP_SCRIPT_UNLOADING";
    else if (asmanip == ASMANIP_ACTORSIMATTR_SET)
        return "ASMANIP_ACTORSIMATTR_SET";
    else
        return "<unknown>";
}

string ScriptCategoryToString(int category)
{
    if (category == SCRIPT_CATEGORY_INVALID)
        return "SCRIPT_CATEGORY_INVALID";
    else if (category == SCRIPT_CATEGORY_ACTOR)
        return "SCRIPT_CATEGORY_ACTOR";
    else if (category == SCRIPT_CATEGORY_TERRAIN)
        return "SCRIPT_CATEGORY_TERRAIN";
    else if (category == SCRIPT_CATEGORY_CUSTOM)
        return "SCRIPT_CATEGORY_CUSTOM";
    else
        return "<unknown>";
}

void main()
{
    log("Script that shows how to use manipulation events.");
    game.registerForEvent(SE_ANGELSCRIPT_MANIPULATIONS);
}

void eventCallbackEx(scriptEvents ev, int a1, int a2, int a3, int a4, string a5, string a6, string a7, string a8)
{
    if (ev == SE_ANGELSCRIPT_MANIPULATIONS)
    {
        log("Received manipulation event " + ManipEventToString(a1) + ":");
        string isItMe = a2 == thisScript ? " (me!)" : "";
        if (a1 == ASMANIP_SCRIPT_LOADED)
        {
            log(" - Script unit ID: " + formatInt(a2) + isItMe);
            log(" - Script category: " + ScriptCategoryToString(a3));
            log(" - Script name: " + a5);
        }
        else if (a1 == ASMANIP_SCRIPT_UNLOADING)
        {
            log(" - Script unit ID: " + formatInt(a2) + isItMe);
            log(" - Script category: " + ScriptCategoryToString(a3));
            log(" - Script name: " + a5);
            // Check if we are about to be unloaded.
            if (a2 == thisScript)
            {
                // You can release all the resources used by your script here.
                log("Releasing resources...");
            }
        }
        else if (a1 == ASMANIP_ACTORSIMATTR_SET)
        {
            log(" - Attribute number: " + formatInt(a2) + isItMe);
            log(" - Attribute name: " + a5);
        }
    }
}
