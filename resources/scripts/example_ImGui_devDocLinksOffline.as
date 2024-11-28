/// \title QUICK SCRIPT DOC UI (Offline)
/// \brief Hand-crafted offline script docs with links to <developer.rigsofrods.org>
/// The local metadata are stored in 2 arrays:
/// * array<DataType@> gDataTypes_ALL ~ Covers value/ref types + singleton names.
/// * arrat<Func@> gFuncs_ALL ~ Covers global funcs + methods.
// ===================================================


// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

//#region Metadata (DB)

const int TYPEFLAG_asOBJ_REF = 1 << 0;
const int TYPEFLAG_asOBJ_NOCOUNT = 1 << 1;
const int TYPEFLAG_BUILTIN = 1 << 2;
array<DataType@> gDataTypes_ALL = array<DataType@>(); // < Filled by DataType constructor, do not push manually.

array<Func@> gFuncs_ALL = array<Func@ >(); // < Filled by FuncDef constructor, do not push manually.

// Primitives
DataType@ gDataTypes_string = DataType("string", "http://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_stdlib_string.html", "built-in string", TYPEFLAG_BUILTIN);
DataType@ gDataTypes_bool = DataType("bool", "http://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_primitives.html#bool", "primitive bool", TYPEFLAG_BUILTIN);
DataType@ gDataTypes_int = DataType("int", "http://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_primitives.html#int", "primitive int", TYPEFLAG_BUILTIN);
DataType@ gDataTypes_dictionary = DataType("dictionary", "http://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_dictionary.html", "built-in dictionary", TYPEFLAG_BUILTIN);

// General
DataType@ gDataTypes_BeamClass = DataType("BeamClass", "https://developer.rigsofrods.org/d8/da5/class_script2_game_1_1_beam_class.html", "A loaded 'actor' (vehicle/load/machine)", TYPEFLAG_asOBJ_REF);
DataType@ gDataTypes_TerrainClass = DataType("TerrainClass", "https://developer.rigsofrods.org/d8/deb/class_script2_game_1_1_terrain_class.html", "A loaded terrain", TYPEFLAG_asOBJ_REF);
DataType@ gDataTypes_CVarClass = DataType("CVarClass", "https://developer.rigsofrods.org/d8/de9/class_script2_game_1_1_c_var_class.html", "Console variable, from RoR.cfg or created by user/script", TYPEFLAG_asOBJ_REF | TYPEFLAG_asOBJ_NOCOUNT);
DataType@ gDataTypes_CacheEntryClass = DataType("CacheEntryClass", "", "Installed mod entry", TYPEFLAG_asOBJ_REF);

// Singletons (with methods)
DataType@ gDataTypes_GameScriptClass = DataType("GameScriptClass", "https://developer.rigsofrods.org/dc/d63/class_script2_game_1_1_game_script_class.html", "Game events, message queue, vehicle and terrain data", TYPEFLAG_asOBJ_REF | TYPEFLAG_asOBJ_NOCOUNT, "game");
Func@ gFuncDefs_getCurrentTruck = Func(gDataTypes_GameScriptClass, "getCurrentTruck", {}, gDataTypes_BeamClass, "Get player's current vehicle, returns <null> if on foot.");
Func@ gFuncs_game_getTerrain = Func(gDataTypes_GameScriptClass, "getTerrain", {}, gDataTypes_TerrainClass, "Get currently loaded terrain, returns <null> if in main menu.");

DataType@ gDataTypes_ConsoleClass = DataType("ConsoleClass", "https://developer.rigsofrods.org/d3/d4f/class_script2_game_1_1_console_class.html",  "Console variables - read and write", TYPEFLAG_asOBJ_REF | TYPEFLAG_asOBJ_NOCOUNT, "console");
Func@ gFunc_console_cVarGet = Func(gDataTypes_ConsoleClass, "cVarGet", { FuncArg(gDataTypes_string, "name", 0), FuncArg(gDataTypes_int, "flags", 0) }, gDataTypes_CVarClass, "Gets or creates a console var");
Func@ gFunc_console_cVarFinds = Func(gDataTypes_ConsoleClass, "cVarFind", { FuncArg(gDataTypes_string, "name", 0) }, gDataTypes_CVarClass, "Gets an existing console var, or returns <null>");

DataType@ gDataTypes_InputEngineClass = DataType("InputEngineClass", "https://developer.rigsofrods.org/d2/d8d/class_script2_game_1_1_input_engine_class.html", "Input keybinds and keystates",TYPEFLAG_asOBJ_REF | TYPEFLAG_asOBJ_NOCOUNT,"inputs");
Func@ gFuncs_inputs_getEventCommandTrimmed = Func(gDataTypes_InputEngineClass, "getEventCommandTrimmed", { FuncArg(gDataTypes_int, "eventID", 0) }, gDataTypes_string, "Get keybind for input event");

DataType@ gDataTypes_CacheSystemClass = DataType("CacheSystemClass", "", "Installed mods - query or create",TYPEFLAG_asOBJ_REF | TYPEFLAG_asOBJ_NOCOUNT, "modcache");
Func@ gFuncs_modcache_findEntryByFilename = Func(gDataTypes_CacheSystemClass, "findEntryByFilename", { FuncArg(gDataTypes_int, "type", 0), FuncArg(gDataTypes_bool, "partial", 0), FuncArg(gDataTypes_string, "name", 0) }, gDataTypes_CacheEntryClass, "Find single mod by fulltext");

//#endregion

//#region DETAIL: class DataType
class DataType
{
    string name;
    string description;
    string singleton_name; //!< The global var name, only selected data types
    string url;
    int typeflags;
    
    DataType(string _n, string _u, string _desc, int _typeflags, string _singleton = "")
    {
        name = _n;
        url = _u;
        description = _desc;
        typeflags = _typeflags;
        singleton_name = _singleton;
        
        gDataTypes_ALL.insertLast(this);
    }
    void drawClassInfo()
    {
        ImGui::Bullet();
        if (url != "")
        {
            imgui_utils::ImHyperlink( url, name, false);
        }
        else // oops, undocumented stuff!!
        {
            ImGui::Text(name);
        }
        if (typeflags & TYPEFLAG_asOBJ_REF != 0)
        {
            ImGui::SameLine(); ImGui::Text("[ref]");
        }
        if (typeflags & TYPEFLAG_asOBJ_NOCOUNT != 0)
        {
            ImGui::SameLine(); ImGui::Text("[nocount]");
        }
        ImGui::SameLine(); ImGui::TextDisabled(description);
    }
    void drawSingletonInfo()
    {
        bool expanded = ImGui::TreeNode("##"+name+url);
        ImGui::SameLine();
        ImGui::SameLine();   
        if (url != "")
        {
            imgui_utils:: ImHyperlink( url, singleton_name, false);
        }
        else // oops, undocumented stuff!!
        {
            ImGui::Text(singleton_name);
        }
        if (expanded)
        {
            for (uint i = 0; i < gFuncs_ALL.length(); i++)
            {
                Func@ func = gFuncs_ALL[i];
                if (@func.obj == @this)
                {
                    func.drawFunc();
                }
            }
            ImGui::TreePop();
        }
        else
        {
            ImGui::SameLine();    ImGui::TextDisabled(description);    
        }
    }
    void drawAsArg(string arg_name)
    {
        imgui_utils:: ImHyperlink( url, name, false);
        ImGui::SameLine(); ImGui::Text(arg_name);
        
    }
    void drawAsRetval()
    {
        imgui_utils:: ImHyperlink( url, name, false);
    }
};

//#endregion (DETAIL: class DataType)

//#region DETAIL: class Func


class Func
{
    DataType@ obj; // null if it's a global function
    DataType@ returns;
    string name;
    string description;
    array<FuncArg@> args;
    
    Func(DataType@ _obj, string _n, array<FuncArg@> _args, DataType@ _ret, string _desc)
    {
        @obj = @_obj;
        name = _n;
        args = _args;
        @returns = @_ret;
        description = _desc;
        gFuncs_ALL.insertLast(this);
    }
    void drawFunc()
    {
        ImGui::Text(" ." + name + "(");
        for (uint i = 0; i < args.length(); i++)
        {
            FuncArg@ farg = args[i];
            ImGui::SameLine(); farg.type.drawAsArg(farg.name);
        }
        ImGui::SameLine(); ImGui::Text(") -> ");
        ImGui::SameLine(); returns.drawAsRetval();
        ImGui::SameLine(); ImGui::TextDisabled(description);
    }
};

class FuncArg
{
    DataType@ type;
    string name;
    int flags = 0;
    FuncArg(DataType@ _t, string _n, int _flags)
    {
        name = _n; 
        @type = @_t;
        flags = _flags;
    }
    
};
//#endregion (DETAIL: class Func)

//#region UI drawing
bool gTypesShowAll = false;
void drawScriptQuickLinksUI()
{
    ImGui::TextDisabled(" ::: G L O B A L S ::: ");
    for (uint i = 0; i < gDataTypes_ALL.length(); i++)
    {
        if ( gDataTypes_ALL[i].singleton_name != "")
        {
            gDataTypes_ALL[i].drawSingletonInfo();   
        }
    }       
    ImGui::Text("");
    
    ImGui::TextDisabled(" ::: T Y P E S ::: ");
    ImGui::SameLine(); ImGui::Checkbox("Show all", gTypesShowAll);
    for (uint i = 0; i < gDataTypes_ALL.length(); i++)
    {
        DataType@ datatype = gDataTypes_ALL[i];
        // by default do not draw builtins and singleton-classes
        const bool builtin = (datatype.typeflags & TYPEFLAG_BUILTIN) != 0;
        const bool singleton = datatype.singleton_name != "";
        if ( (!builtin && !singleton) || gTypesShowAll )
        {
            gDataTypes_ALL[i].drawClassInfo();
        }
    }
    ImGui::Text("");
    
    
    
    // `modcache`
    //  ImGui::Text(" .findEntryByFilename(/*loaderType*/LOADER_TYPE_ALLBEAM, /*partial:*/false, actor.getTruckFileName());");
    
}

//  ~~~ Script quick links ~~~ HELPERS ~~~ //

void drawFunc(string name, string ret_obj, string ret_url_local)
{
    ImGui::Text(name + "()");
    ImGui::SameLine();
}



//#endregion (UI drawing)

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    
    if (ImGui::Begin("Scripting - quick links UI", closeBtnHandler.windowOpen, 0))
    {
        ImGui::TextDisabled(" * * OFFLINE EDITION * * (Only popular APIs) * * ");
        ImGui::Separator();
        drawScriptQuickLinksUI();    
        ImGui::End(); 
    }
}

