/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author Thomas Fischer thomas{AT}thomasfischer{DOT}biz
/// @date   19th of October 2011


#include "DashBoardManager.h"

#include "Actor.h"
#include "Application.h"
#include "CacheSystem.h"
#include "Console.h"
#include "GenericFileFormat.h"
#include "ScriptEngine.h"
#include "Utils.h"

using namespace Ogre;
using namespace RoR;

#define INITDATA(key, type, name) data[key] = dashData_t(type, name)

DashBoardManager::DashBoardManager(ActorPtr actor) : visible(true), m_actor(actor)
{
    // init data
    INITDATA(DD_ENGINE_RPM              , DC_FLOAT, "rpm");
    INITDATA(DD_ENGINE_SPEEDO_KPH       , DC_FLOAT, "speedo_kph");
    INITDATA(DD_ENGINE_SPEEDO_MPH       , DC_FLOAT, "speedo_mph");
    INITDATA(DD_ENGINE_TURBO            , DC_FLOAT, "engine_turbo");
    INITDATA(DD_ENGINE_IGNITION         , DC_BOOL , "engine_ignition");
    INITDATA(DD_ENGINE_BATTERY          , DC_BOOL , "engine_battery");
    INITDATA(DD_ENGINE_CLUTCH_WARNING   , DC_BOOL , "engine_clutch_warning");
    INITDATA(DD_ENGINE_GEAR             , DC_INT  , "engine_gear");
    INITDATA(DD_ENGINE_NUM_GEAR         , DC_INT  , "engine_num_gear");
    INITDATA(DD_ENGINE_GEAR_STRING      , DC_CHAR , "engine_gear_string");
    INITDATA(DD_ENGINE_AUTOGEAR_STRING  , DC_CHAR , "engine_autogear_string");
    INITDATA(DD_ENGINE_AUTO_GEAR        , DC_INT  , "engine_auto_gear");
    INITDATA(DD_ENGINE_CLUTCH           , DC_FLOAT, "engine_clutch");
    INITDATA(DD_BRAKE                   , DC_FLOAT, "brake");
    INITDATA(DD_ACCELERATOR             , DC_FLOAT, "accelerator");
    INITDATA(DD_ROLL                    , DC_FLOAT, "roll");
    INITDATA(DD_ROLL_CORR               , DC_FLOAT, "roll_corr");
    INITDATA(DD_ROLL_CORR_ACTIVE        , DC_BOOL , "roll_corr_active");
    INITDATA(DD_PITCH                   , DC_FLOAT, "pitch");
    INITDATA(DD_PARKINGBRAKE            , DC_BOOL , "parkingbrake");
    INITDATA(DD_LOCKED                  , DC_BOOL , "locked");
    INITDATA(DD_LOW_PRESSURE            , DC_BOOL , "low_pressure");
    INITDATA(DD_TRACTIONCONTROL_MODE    , DC_INT  , "tractioncontrol_mode");
    INITDATA(DD_ANTILOCKBRAKE_MODE      , DC_INT  , "antilockbrake_mode");
    INITDATA(DD_TIES_MODE               , DC_INT  , "ties_mode");
    INITDATA(DD_SCREW_THROTTLE_0        , DC_FLOAT, "screw_throttle_0");
    INITDATA(DD_SCREW_THROTTLE_1        , DC_FLOAT, "screw_throttle_1");
    INITDATA(DD_SCREW_THROTTLE_2        , DC_FLOAT, "screw_throttle_2");
    INITDATA(DD_SCREW_THROTTLE_3        , DC_FLOAT, "screw_throttle_3");
    INITDATA(DD_SCREW_THROTTLE_4        , DC_FLOAT, "screw_throttle_4");
    INITDATA(DD_SCREW_THROTTLE_5        , DC_FLOAT, "screw_throttle_5");
    INITDATA(DD_SCREW_STEER_0           , DC_FLOAT, "screw_steer_0");
    INITDATA(DD_SCREW_STEER_1           , DC_FLOAT, "screw_steer_1");
    INITDATA(DD_SCREW_STEER_2           , DC_FLOAT, "screw_steer_2");
    INITDATA(DD_SCREW_STEER_3           , DC_FLOAT, "screw_steer_3");
    INITDATA(DD_SCREW_STEER_4           , DC_FLOAT, "screw_steer_4");
    INITDATA(DD_SCREW_STEER_5           , DC_FLOAT, "screw_steer_5");
    INITDATA(DD_WATER_DEPTH             , DC_FLOAT, "water_depth");
    INITDATA(DD_WATER_SPEED             , DC_FLOAT, "water_speed");
    INITDATA(DD_AEROENGINE_THROTTLE_0   , DC_FLOAT, "aeroengine_throttle_0");
    INITDATA(DD_AEROENGINE_THROTTLE_1   , DC_FLOAT, "aeroengine_throttle_1");
    INITDATA(DD_AEROENGINE_THROTTLE_2   , DC_FLOAT, "aeroengine_throttle_2");
    INITDATA(DD_AEROENGINE_THROTTLE_3   , DC_FLOAT, "aeroengine_throttle_3");
    INITDATA(DD_AEROENGINE_THROTTLE_4   , DC_FLOAT, "aeroengine_throttle_4");
    INITDATA(DD_AEROENGINE_THROTTLE_5   , DC_FLOAT, "aeroengine_throttle_5");
    INITDATA(DD_AEROENGINE_FAILED_0     , DC_BOOL , "aeroengine_failed_0");
    INITDATA(DD_AEROENGINE_FAILED_1     , DC_BOOL , "aeroengine_failed_1");
    INITDATA(DD_AEROENGINE_FAILED_2     , DC_BOOL , "aeroengine_failed_2");
    INITDATA(DD_AEROENGINE_FAILED_3     , DC_BOOL , "aeroengine_failed_3");
    INITDATA(DD_AEROENGINE_FAILED_4     , DC_BOOL , "aeroengine_failed_4");
    INITDATA(DD_AEROENGINE_FAILED_5     , DC_BOOL , "aeroengine_failed_5");
    INITDATA(DD_AEROENGINE_RPM_0        , DC_FLOAT, "aeroengine_rpm_0");
    INITDATA(DD_AEROENGINE_RPM_1        , DC_FLOAT, "aeroengine_rpm_1");
    INITDATA(DD_AEROENGINE_RPM_2        , DC_FLOAT, "aeroengine_rpm_2");
    INITDATA(DD_AEROENGINE_RPM_3        , DC_FLOAT, "aeroengine_rpm_3");
    INITDATA(DD_AEROENGINE_RPM_4        , DC_FLOAT, "aeroengine_rpm_4");
    INITDATA(DD_AEROENGINE_RPM_5        , DC_FLOAT, "aeroengine_rpm_5");
    INITDATA(DD_AIRSPEED                , DC_FLOAT, "airspeed");
    INITDATA(DD_WING_AOA_0              , DC_FLOAT, "wing_aoa_0");
    INITDATA(DD_WING_AOA_1              , DC_FLOAT, "wing_aoa_1");
    INITDATA(DD_WING_AOA_2              , DC_FLOAT, "wing_aoa_2");
    INITDATA(DD_WING_AOA_3              , DC_FLOAT, "wing_aoa_3");
    INITDATA(DD_WING_AOA_4              , DC_FLOAT, "wing_aoa_4");
    INITDATA(DD_WING_AOA_5              , DC_FLOAT, "wing_aoa_5");
    INITDATA(DD_ALTITUDE                , DC_FLOAT, "altitude");
    INITDATA(DD_ALTITUDE_STRING         , DC_CHAR , "altitude_string");

    INITDATA(DD_ODOMETER_TOTAL          , DC_FLOAT, "odometer_total");
    INITDATA(DD_ODOMETER_USER           , DC_FLOAT, "odometer_user");

    // Lights (mirrors RoRnet::Lightmask)

    INITDATA(DD_CUSTOM_LIGHT1           , DC_BOOL, "custom_light1");
    INITDATA(DD_CUSTOM_LIGHT2           , DC_BOOL, "custom_light2");
    INITDATA(DD_CUSTOM_LIGHT3           , DC_BOOL, "custom_light3");
    INITDATA(DD_CUSTOM_LIGHT4           , DC_BOOL, "custom_light4");
    INITDATA(DD_CUSTOM_LIGHT5           , DC_BOOL, "custom_light5");
    INITDATA(DD_CUSTOM_LIGHT6           , DC_BOOL, "custom_light6");
    INITDATA(DD_CUSTOM_LIGHT7           , DC_BOOL, "custom_light7");
    INITDATA(DD_CUSTOM_LIGHT8           , DC_BOOL, "custom_light8");
    INITDATA(DD_CUSTOM_LIGHT9           , DC_BOOL, "custom_light9");
    INITDATA(DD_CUSTOM_LIGHT10          , DC_BOOL, "custom_light10");

    INITDATA(DD_HEADLIGHTS              , DC_BOOL, "headlights");
    INITDATA(DD_HIGHBEAMS               , DC_BOOL, "highbeams");
    INITDATA(DD_FOGLIGHTS               , DC_BOOL, "foglights");
    INITDATA(DD_SIDELIGHTS              , DC_BOOL, "sidelights");
    INITDATA(DD_BRAKE_LIGHTS            , DC_BOOL, "brake_lights");
    INITDATA(DD_REVERSE_LIGHT           , DC_BOOL, "reverse_light");
    INITDATA(DD_BEACONS                 , DC_BOOL, "beacons");
    INITDATA(DD_LIGHTS_LEGACY           , DC_BOOL, "lights"); // Alias of 'sidelights'

    INITDATA(DD_SIGNAL_TURNLEFT         , DC_BOOL, "signal_turnleft");
    INITDATA(DD_SIGNAL_TURNRIGHT        , DC_BOOL, "signal_turnright");
    INITDATA(DD_SIGNAL_WARNING          , DC_BOOL, "signal_warning");

    // load dash fonts
    MyGUI::ResourceManager::getInstance().load("MyGUI_FontsDash.xml");
}

DashBoardManager::~DashBoardManager(void)
{
    // free all objects
    while (m_dashboards.size() > 0)
    {
        delete m_dashboards.back();
        m_dashboards.pop_back();
    }
}

int DashBoardManager::registerCustomValue(Ogre::String& name, int dataType)
{
    int newKey = -1;
    bool valid = true;

    if (registeredCustomValues >= DD_MAX_CUSTOM_VALUES)
    {
        valid = false;
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: Cannot register dashboard custom value \"{}\", maximum reached ({} custom values).", m_actor->ar_design_name, name, DD_MAX_CUSTOM_VALUES));
    }
    if (getLinkIDForName(name) != -1)
    {
        valid = false;
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: Cannot register dashboard custom value \"{}\", name conflicts with existing one. Ignoring it.", m_actor->ar_design_name, name));
    }
    if (dataType == DC_INVALID || dataType >= DC_MAX || dataType <= DC_MIN)
    {
        valid = false;
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: Cannot register dashboard custom value \"{}\", invalid data type.", m_actor->ar_design_name, name));
    }

    if (valid)
    {
        newKey = DD_CUSTOMVALUE_START + registeredCustomValues;
        INITDATA(newKey, dataType, name.c_str());
        registeredCustomValues++;
    }

    return newKey;
}

int DashBoardManager::getLinkIDForName(Ogre::String& str)
{
    const char* s = str.c_str();
    for (int i = 0; i < DD_MAX; i++)
    {
        if (!strcmp(data[i].name, s))
            return i;
    }
    return -1;
}

std::string DashBoardManager::getLinkNameForID(DashData id)
{
    if (id > 0 && id < DD_MAX)
    {
        return data[id].name;
    }
    else
    {
        return "";
    }
}

// Helper funcs and structs for `determineLayoutFromDashboardMod()` below.

constexpr int DASHTAG_RPM_NONE = -1;
constexpr char DASHTAG_XPH_NONE = '\0';

static int DashRPM(const std::string& input)
{
    std::regex rpm_regex(R"((\d+)rpm)");
    std::smatch match;
    if (std::regex_search(input, match, rpm_regex)) {
        std::string rpm = match[1];
        return std::atoi(rpm.c_str());
    }
    return DASHTAG_RPM_NONE;
}

static char DashXPH(const std::string& input)
{
    std::regex xph_regex(R"(([km])ph)");
    std::smatch match;
    if (std::regex_search(input, match, xph_regex)) {
        return match[1].str()[0];
    }
    return DASHTAG_XPH_NONE;
}

struct DashCandidateLayout
{
    std::string filename;
    int rpm = DASHTAG_RPM_NONE;
    char xph = DASHTAG_XPH_NONE;

    DashCandidateLayout(const std::string& filename)
    {
        this->filename = filename;
        this->rpm = DashRPM(filename);
        this->xph = DashXPH(filename);
    }
};

std::string DashBoardManager::determineLayoutFromDashboardMod(CacheEntryPtr& entry, std::string const& basename)
{
    App::GetCacheSystem()->LoadResource(entry);
    Ogre::FileInfoListPtr filelist
        = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo(entry->resource_group, fmt::format("{}*.layout", basename));

    if (filelist->empty())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: No layout files found in dashboard '{}'", m_actor->ar_design_name, basename));
        return "";
    }

    // Boat and aircraft dashboards are separate from trucks and have no tags to evaluate.
    if (m_actor->ar_driveable == BOAT ||
        m_actor->ar_driveable == AIRPLANE)
    {
        return filelist->begin()->filename;
    }

    if (m_actor->ar_driveable == TRUCK)
    {
        return this->determineTruckLayoutFromDashboardMod(filelist);
    }

    return "";
}

std::string DashBoardManager::determineTruckLayoutFromDashboardMod(Ogre::FileInfoListPtr& filelist)
{
    // Algorithm:
    // A. Consider only layouts with matching Xph tag, find best RPM match (see below).
    // B. If no match found, consider also layouts without Xph tag, find best RPM match (see below).
    // 
    // Best RPM matching:
    // 1. Consider just layouts with Xrpm tag, find one with with smallest RPM overshoot.
    // 2. If all undershoot, take one with least undershoot and log a warning
    // 3. If there's no layout with Xrpm tag, Consider layouts without Xrpm tag, pick random .
    // ---------------------------------------------------------------------------------------

    const int redlineRPM = (int)m_actor->ar_engine->getShiftUpRPM();
    const char desiredX = App::gfx_speedo_imperial->getBool() ? 'm' : 'k';
    std::vector<DashCandidateLayout> candidates;

    for (Ogre::FileInfo& fileinfo : *filelist)
    {
        candidates.emplace_back(fileinfo.filename);
    }

    // A. Consider only layouts with matching Xph tag, find best RPM match (see above).
    float least_overshoot = std::numeric_limits<float>::max(); DashCandidateLayout* overshoot_candidate = nullptr;
    float least_undershoot = -std::numeric_limits<float>::max(); DashCandidateLayout* undershoot_candidate = nullptr;
    for (auto& candidate : candidates)
    {
        if (candidate.xph == desiredX)
        {
            float rpm_diff = (float)candidate.rpm - redlineRPM;
            if (rpm_diff < 0 && rpm_diff > least_undershoot)
            {
                least_undershoot = rpm_diff;
                undershoot_candidate = &candidate;
            }
            else if (rpm_diff >= 0 && rpm_diff < least_overshoot)
            {
                least_overshoot = rpm_diff;
                overshoot_candidate = &candidate;
            }
        }
    }

    if (overshoot_candidate)
    {
        return overshoot_candidate->filename;
    }
    else if (undershoot_candidate)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: No ideal dashboard found, using one with least RPM undershoot", m_actor->ar_design_name));
        return undershoot_candidate->filename;
    }

    // B. If no match found, consider also layouts without Xph tag, find best RPM match (see above).
    App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
        fmt::format("{}: Selected dashboard has no '{}ph' layouts, ignoring setting", m_actor->ar_design_name, desiredX));
    least_overshoot = std::numeric_limits<float>::max(); overshoot_candidate = nullptr;
    least_undershoot = -std::numeric_limits<float>::max(); undershoot_candidate = nullptr;
    for (auto& candidate : candidates)
    {
        float rpm_diff = (float)candidate.rpm - redlineRPM;
        if (rpm_diff < 0 && rpm_diff > least_undershoot)
        {
            least_undershoot = rpm_diff;
            undershoot_candidate = &candidate;
        }
        else if (rpm_diff >= 0 && rpm_diff < least_overshoot)
        {
            least_overshoot = rpm_diff;
            overshoot_candidate = &candidate;
        }
    }

    if (overshoot_candidate)
    {
        return overshoot_candidate->filename;
    }
    else if (undershoot_candidate)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: No ideal dashboard found, using one with least RPM undershoot", m_actor->ar_design_name));
        return undershoot_candidate->filename;
    }

    return filelist->begin()->filename; // If all else failed, just pick random.
}

void DashBoardManager::loadDashBoard(std::string const& filename, BitMask_t flags)
{
    // filename may be either '.layout' file (classic approach) or a new '.dashboard' mod.
    // ----------------------------------------------------------------------------------

    if (BITMASK_IS_0(flags, LOADDASHBOARD_SCREEN_HUD | LOADDASHBOARD_RTT_TEXTURE))
        return; // Nothing to do.

    std::string basename, ext, layoutfname, scriptfilename = "";
    Ogre::StringUtil::splitBaseFilename(filename, basename, ext);
    if (ext == "dashboard")
    {
        CacheEntryPtr entry = App::GetCacheSystem()->FindEntryByFilename(LT_DashBoard, /*partial=*/false, filename);
        if (!entry)
        {
            App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
                fmt::format("DashboardManager: Could not find dashboard file '{}'", filename));
            return;
        }
        App::GetCacheSystem()->LoadResource(entry);
        layoutfname = this->determineLayoutFromDashboardMod(entry, basename);

        // Load custom inputs
        for (DashboardCustomInput& custom_input : entry->custom_dashboard_inputs)
        {
            this->registerCustomValue(custom_input.name, custom_input.dataType);
        }

        // load dash fonts
        Ogre::FileInfoListPtr filelist
            = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo(entry->resource_group, fmt::format("{}*.resource", basename));
        for (Ogre::FileInfo& fileinfo : *filelist)
        {
            MyGUI::ResourceManager::getInstance().load(fileinfo.filename);
        }

        Ogre::FileInfoListPtr scriptFile
            = Ogre::ResourceGroupManager::getSingleton().findResourceFileInfo(entry->resource_group, fmt::format("{}.as", basename));
        if (scriptFile->size() > 0)
        {
            scriptfilename = scriptFile->front().filename;
        }
    }
    else
    {
        layoutfname = filename;
    }

    if (layoutfname == "")
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("{}: Cannot load dashboard '{}' - no applicable layout file found", m_actor->ar_design_name, filename));
        return;
    }

    DashBoard* new_dash = nullptr;

    if (BITMASK_IS_1(flags, LOADDASHBOARD_RTT_TEXTURE))
    {
        new_dash = new DashBoard(this, layoutfname, loadedRTTDashboards + 1);
        loadedRTTDashboards++;
    }

    if (BITMASK_IS_1(flags, LOADDASHBOARD_SCREEN_HUD))
    {
        new_dash = new DashBoard(this, layoutfname, NO_RTT_DASHBOARD);
    }

    if (new_dash != nullptr)
    {
        new_dash->setVisible(true);
        
        if (scriptfilename != "")
        {
            new_dash->loadScript(scriptfilename, m_actor);
        }

        m_dashboards.push_back(new_dash);
        if (BITMASK_IS_0(flags, LOADDASHBOARD_STACKABLE))
        {
            m_rtt_loaded = true;
        }
    }
}

void DashBoardManager::update(float dt)
{
    for (DashBoard* d : m_dashboards)
    {
        d->update(dt);
    }
}

void DashBoardManager::updateFeatures()
{
    for (DashBoard* d : m_dashboards)
    {
        d->updateFeatures();
    }
}

float DashBoardManager::getNumeric(size_t key)
{
    if (key >= DD_MAX)
        return 0;

    switch (data[key].type)
    {
    case DC_BOOL:
        return data[key].data.value_bool ? 1.0f : 0.0f;
    case(DC_INT):
        return (float)data[key].data.value_int;
    case(DC_FLOAT):
        return data[key].data.value_float;
    }
    return 0;
}

void DashBoardManager::setVisible(bool visibility)
{
    visible = visibility;
    for (DashBoard* d : m_dashboards)
    {
        if (!d->getIsTextureLayer())
        {
            d->setVisible(visibility);
        }
    }
}

void DashBoardManager::setVisible3d(bool visibility)
{
    for (DashBoard* d : m_dashboards)
    {
        if (d->getIsTextureLayer())
        {
            d->setVisible(visibility, /*smooth:*/false);
        }
    }
}

void DashBoardManager::windowResized()
{
    for (DashBoard* d : m_dashboards)
    {
        d->windowResized();
    }
}

// DASHBOARD class below

DashBoard::DashBoard(DashBoardManager* manager, Ogre::String filename, int _textureLayerNum)
    : manager(manager)
    , filename(filename)
    , free_controls(0)
    , visible(false)
    , mainWidget(nullptr)
    , textureLayerNum(_textureLayerNum)
{
    // use 'this' class pointer to make layout unique
    prefix = MyGUI::utility::toString(this, "_");

    if (getIsTextureLayer())
    {
        rttLayer = fmt::format("RTTLayer{}", textureLayerNum);
        rttTexture = fmt::format("RTTTexture{}", textureLayerNum);
    }

    memset(&controls, 0, sizeof(controls));
    loadLayoutInternal();
    // hide first
    if (mainWidget)
        mainWidget->setVisible(false);
}

DashBoard::~DashBoard()
{
    // Unload dashboard script
    if (scriptUnitID != SCRIPTUNITID_INVALID)
    {
        App::GetScriptEngine()->unloadScript(scriptUnitID);
    }
    // Clear the GUI widgets
    MyGUI::LayoutManager::getInstance().unloadLayout(widgets);
    // Force unloading the '.layout' file from memory
    MyGUI::ResourceManager::getInstance().removeByName(filename);
}

void DashBoard::loadScript(std::string scriptFilename, ActorPtr associatedActor)
{
    if (scriptUnitID != SCRIPTUNITID_INVALID)
        return;

    scriptUnitID = App::GetScriptEngine()->loadScript(scriptFilename, ScriptCategory::ACTOR, associatedActor);
}

void DashBoard::updateFeatures()
{
    // this hides / shows parts of the gui depending on the vehicle features
    for (int i = 0; i < free_controls; i++)
    {
        bool enabled = manager->getEnabled(controls[i].linkIDForVisibility);

        controls[i].widget->setVisible(enabled);
    }
}

const float DASH_SMOOTHING = 0.02;

float DashBoard::getSmoothNumeric(int linkID, float& lastVal)
{
    if (manager->getDataType(linkID) != DC_FLOAT)
    {
        return manager->getNumeric(linkID); // Only smoothen FLOAT inputs
    }
    else
    {
        const float curVal = manager->getNumeric(linkID);
        const float val = lastVal * (1 - DASH_SMOOTHING) + curVal * DASH_SMOOTHING;
        lastVal = curVal;
        return val;
    }
}

void DashBoard::update(float dt)
{
    // walk all controls and animate them
    for (int i = 0; i < free_controls; i++)
    {
        // Process graphical animation
        if (controls[i].graphicalAnimation.animationType == ANIM_LAMP)
        {
            // or a lamp?
            bool state = false;
            // conditional
            if (controls[i].graphicalAnimation.condition == CONDITION_GREATER)
            {
                float val = manager->getNumeric(controls[i].graphicalAnimation.linkID);
                state = (val > controls[i].graphicalAnimation.conditionArgument);
            }
            else if (controls[i].graphicalAnimation.condition == CONDITION_LESSER)
            {
                float val = manager->getNumeric(controls[i].graphicalAnimation.linkID);
                state = (val < controls[i].graphicalAnimation.conditionArgument);
            }
            else
            {
                state = (manager->getNumeric(controls[i].graphicalAnimation.linkID) > 0);
            }

            if (state == controls[i].lastState)
                continue;
            controls[i].lastState = state;

            // switch states
            if (state)
            {
                controls[i].img->setImageTexture(String(controls[i].graphicalAnimation.texture) + "-on.png");
            }
            else
            {
                controls[i].img->setImageTexture(String(controls[i].graphicalAnimation.texture) + "-off.png");
            }
        }
        else if (controls[i].graphicalAnimation.animationType == ANIM_SERIES)
        {
            const float val = this->getSmoothNumeric(controls[i].graphicalAnimation.linkID, controls[i].graphicalAnimation.lastVal);

            String fn = String(controls[i].graphicalAnimation.texture) + String("-") + TOSTRING((int)val) + String(".png");

            controls[i].img->setImageTexture(fn);
        }
        else if (controls[i].graphicalAnimation.animationType == ANIM_TEXTFORMAT)
        {
            const float val = this->getSmoothNumeric(controls[i].graphicalAnimation.linkID, controls[i].graphicalAnimation.lastVal);

            MyGUI::UString s;
            if (strlen(controls[i].graphicalAnimation.format) == 0)
            {
                s = Ogre::StringConverter::toString(val);
            }
            else
            {
                char tmp[1024] = "";
                sprintf(tmp, controls[i].graphicalAnimation.format, val);

                // Detect and eliminate negative zero (-0) on output
                if (strcmp(tmp, controls[i].graphicalAnimation.format_neg_zero) == 0)
                {
                    sprintf(tmp, controls[i].graphicalAnimation.format, 0.f);
                }

                s = MyGUI::UString(tmp);
            }

            controls[i].txt->setCaption(s);
        }
        else if (controls[i].graphicalAnimation.animationType == ANIM_TEXTSTRING)
        {
            char* val = manager->getChar(controls[i].graphicalAnimation.linkID);
            controls[i].txt->setCaption(MyGUI::UString(val));
        }

        float finalHorizontalTranslation = 0,
            finalVerticalTranslation = 0;
        for (int animIdx = 0; animIdx < controls[i].geometricAnimationCount; animIdx++)
        {
            layoutGeometricAnimation_t& animation = controls[i].geometricAnimations[animIdx];

            // get its value from its linkage
            if (animation.animationType == ANIM_ROTATE)
            {
                // get the value
                const float val = this->getSmoothNumeric(animation.linkID, animation.lastVal);
                // calculate the angle
                float angle = (val - animation.vmin) * (animation.wmax - animation.wmin) / (animation.vmax - animation.vmin) + animation.wmin;

                // enforce limits
                if (angle < animation.wmin)
                    angle = animation.wmin;
                else if (angle > animation.wmax)
                    angle = animation.wmax;
                // rotate finally
                controls[i].rotImg->setAngle(Ogre::Degree(angle).valueRadians());
            }
            else if (animation.animationType == ANIM_SCALE)
            {
                const float val = this->getSmoothNumeric(animation.linkID, animation.lastVal);

                float scale = (val - animation.vmin) * (animation.wmax - animation.wmin) / (animation.vmax - animation.vmin) + animation.wmin;
                if (animation.direction == DIRECTION_UP)
                {
                    controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top - scale);
                    controls[i].widget->setSize(controls[i].initialSize.width, controls[i].initialSize.height + scale);
                }
                else if (animation.direction == DIRECTION_DOWN)
                {
                    controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top);
                    controls[i].widget->setSize(controls[i].initialSize.width, controls[i].initialSize.height + scale);
                }
                else if (animation.direction == DIRECTION_LEFT)
                {
                    controls[i].widget->setPosition(controls[i].initialPosition.left - scale, controls[i].initialPosition.top);
                    controls[i].widget->setSize(controls[i].initialSize.width + scale, controls[i].initialSize.height);
                }
                else if (animation.direction == DIRECTION_RIGHT)
                {
                    controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top);
                    controls[i].widget->setSize(controls[i].initialSize.width + scale, controls[i].initialSize.height);
                }
            }
            else if (animation.animationType == ANIM_TRANSLATE)
            {
                const float val = this->getSmoothNumeric(animation.linkID, animation.lastVal);

                float translation = (val - animation.vmin) * (animation.wmax - animation.wmin) / (animation.vmax - animation.vmin) + animation.wmin;
                if (animation.direction == DIRECTION_UP)
                    finalVerticalTranslation -= translation;
                else if (animation.direction == DIRECTION_DOWN)
                    finalVerticalTranslation += translation;
                else if (animation.direction == DIRECTION_LEFT)
                    finalHorizontalTranslation -= translation;
                else if (animation.direction == DIRECTION_RIGHT)
                    finalHorizontalTranslation += translation;
            }
        }

        controls[i].widget->setPosition(controls[i].initialPosition.left + finalHorizontalTranslation, controls[i].initialPosition.top + finalVerticalTranslation);
    }
}

void DashBoard::windowResized()
{
    if (!mainWidget)
        return;
    mainWidget->setPosition(0, 0);
    if (getIsTextureLayer())
    {
        // texture layers are independent from the screen size, but rather from the layer texture size
        TexturePtr tex = TextureManager::getSingleton().getByName(rttTexture);
        if (tex)
            mainWidget->setSize(tex->getWidth(), tex->getHeight());
    }
    else
    {
        MyGUI::IntSize screenSize = MyGUI::RenderManager::getInstance().getViewSize();
        mainWidget->setSize(screenSize);
    }
}

bool DashBoard::parseLink(std::string& linkArgs, int& linkID, char& condition, float& conditionArgument, const std::string& filename, const std::string& name)
{
    replaceString(linkArgs, "&gt;", ">");
    replaceString(linkArgs, "&lt;", "<");
    String linkName = "";
    if (linkArgs.empty())
    {
        LOG("Dashboard (" + filename + "/" + name + "): empty Link");
        return false;
    }
    // conditional checks
    // TODO: improve the logic, this is crap ...
    if (linkArgs.find(">") != linkArgs.npos)
    {
        Ogre::StringVector args = Ogre::StringUtil::split(linkArgs, ">");
        if (args.size() == 2)
        {
            linkName = args[0];
            conditionArgument = StringConverter::parseReal(args[1]);
            condition = CONDITION_GREATER;
        }
        else
        {
            LOG("Dashboard (" + filename + "/" + name + "): error in conditional Link: " + linkArgs);
            return false;
        }
    }
    else if (linkArgs.find("<") != linkArgs.npos)
    {
        Ogre::StringVector args = Ogre::StringUtil::split(linkArgs, "<");
        if (args.size() == 2)
        {
            linkName = args[0];
            conditionArgument = StringConverter::parseReal(args[1]);
            condition = CONDITION_LESSER;
        }
        else
        {
            LOG("Dashboard (" + filename + "/" + name + "): error in conditional Link: " + linkArgs);
            return false;
        }
    }
    else
    {
        condition = CONDITION_NONE;
        conditionArgument = 0;
        linkName = linkArgs;
    }

    // now try to get the enum id for it
    int linkIDResult = manager->getLinkIDForName(linkName);
    if (linkIDResult < 0)
    {
        LOG("Dashboard (" + filename + "/" + name + "): unknown Link: " + linkName);
        return false;
    }

    linkID = linkIDResult;
    return true;
}

void DashBoard::loadLayoutRecursive(MyGUI::WidgetPtr w)
{
    std::string name = w->getName();
    std::string debug = w->getUserString("debug");

    // make it unclickable
    w->setUserString("interactive", "0");

    if (!debug.empty())
    {
        w->setVisible(false);
        return;
    }

    // find the root widget and ignore debug widgets
    if (name.size() > prefix.size())
    {
        std::string prefixLessName = name.substr(prefix.size());
        if (prefixLessName == "_Main")
        {
            mainWidget = (MyGUI::WindowPtr)w;
            // resize it
            windowResized();
        }

        // ignore debug widgets
        if (prefixLessName == "DEBUG")
        {
            w->setVisible(false);
            return;
        }
    }

    layoutLink_t ctrl;
    memset(&ctrl, 0, sizeof(ctrl));

    int linkID;
    char condition;
    float conditionArgument;

    // Variable used to determine if one of the following
    // animations is used:
    // - series
    // - textcolor/textcolour
    // - textformat
    // - textstring
    // - lamp
    // Only one of them can be specified for each widget.
    std::string graphicalAnimationUsed = "";

    int animNum = 1;
    // Numbered properties (anim, anim2, link, link2, etc.)
    // Allows to define multiple animations for a widget.
    std::string animNumStr = "anim";
    std::string linkNumStr = "link";
    std::string minNumStr = "min";
    std::string maxNumStr = "max";
    std::string vminNumStr = "vmin";
    std::string vmaxNumStr = "vmax";
    std::string textureNumStr = "texture";
    std::string formatNumStr = "format";
    std::string directionNumStr = "direction";

    std::string anim = w->getUserString(animNumStr);
    std::string linkArgs;
    if (anim != "")
    {
        bool linkIDForVisibilitySet = false;
        while (anim != "")
        {
            linkArgs = w->getUserString(linkNumStr);

            // animations for this control?
            if (!linkArgs.empty())
            {
                char animType;
                bool graphicalAnimation = false,
                    graphicalAnimationAlreadySet = false,
                    geometricAnimation = false;
                if (anim == "rotate")
                {
                    animType = ANIM_ROTATE;
                    geometricAnimation = true;
                }
                else if (anim == "scale")
                {
                    animType = ANIM_SCALE;
                    geometricAnimation = true;
                }
                else if (anim == "translate")
                {
                    animType = ANIM_TRANSLATE;
                    geometricAnimation = true;
                }
                else if (anim == "series")
                {
                    animType = ANIM_SERIES;
                    graphicalAnimation = true;
                    if (graphicalAnimationUsed == "")
                        graphicalAnimationUsed = anim;
                    else
                        graphicalAnimationAlreadySet = true;
                }
                else if (anim == "textcolor" || anim == "textcolour")
                {
                    animType = ANIM_TEXTCOLOR;
                    graphicalAnimation = true;
                    if (graphicalAnimationUsed == "")
                        graphicalAnimationUsed = anim;
                    else
                        graphicalAnimationAlreadySet = true;
                }
                else if (anim == "textformat")
                {
                    animType = ANIM_TEXTFORMAT;
                    graphicalAnimation = true;
                    if (graphicalAnimationUsed == "")
                        graphicalAnimationUsed = anim;
                    else
                        graphicalAnimationAlreadySet = true;
                }
                else if (anim == "textstring")
                {
                    animType = ANIM_TEXTSTRING;
                    graphicalAnimation = true;
                    if (graphicalAnimationUsed == "")
                        graphicalAnimationUsed = anim;
                    else
                        graphicalAnimationAlreadySet = true;
                }
                else if (anim == "lamp")
                {
                    animType = ANIM_LAMP;
                    graphicalAnimation = true;
                    if (graphicalAnimationUsed == "")
                        graphicalAnimationUsed = anim;
                    else
                        graphicalAnimationAlreadySet = true;
                }
                else
                {
                    LOG("Dashboard (" + filename + "/" + name + "): Unknown animation \"" + anim + "\".");
                    return;
                }

                if (graphicalAnimationAlreadySet)
                {
                    LOG("Dashboard (" + filename + "/" + name + "): Animations \"" + anim + "\" and \"" + graphicalAnimationUsed + "\" can't be used at the same time.");
                    return;
                }

                ctrl.widget = w;
                ctrl.initialSize = w->getSize();
                ctrl.initialPosition = w->getPosition();
                ctrl.lastState = false;
                if (!name.empty())
                    strncpy(ctrl.name, name.c_str(), 255);

                bool linkParsed = parseLink(linkArgs, linkID, condition, conditionArgument, filename, name);
                if (!linkParsed)
                    return;

                // The first animation will determine the visibility of the widget.
                if (!linkIDForVisibilitySet)
                {
                    ctrl.linkIDForVisibility = linkID;
                    linkIDForVisibilitySet = true;
                }

                if (graphicalAnimation)
                {
                    ctrl.graphicalAnimation.linkID = linkID;
                    ctrl.graphicalAnimation.animationType = animType;
                    ctrl.graphicalAnimation.condition = condition;
                    ctrl.graphicalAnimation.conditionArgument = conditionArgument;

                    String texture = w->getUserString(textureNumStr);
                    if (!texture.empty())
                        strncpy(ctrl.graphicalAnimation.texture, texture.c_str(), 255);

                    String format = w->getUserString(formatNumStr);
                    if (!format.empty())
                        strncpy(ctrl.graphicalAnimation.format, format.c_str(), 255);

                    if (animType == ANIM_SERIES)
                    {
                        ctrl.img = (MyGUI::ImageBox*)w; //w->getSubWidgetMain()->castType<MyGUI::ImageBox>();
                        if (!ctrl.img)
                        {
                            LOG("Dashboard (" + filename + "/" + name + "): error loading series control");
                            return;
                        }
                    }
                    else if (animType == ANIM_TEXTCOLOR)
                    {
                        // try to cast, will throw
                        try
                        {
                            ctrl.txt = (MyGUI::TextBox*)w;
                        }
                        catch (...)
                        {
                            LOG("Dashboard (" + filename + "/" + name + "): textcolor controls must use the TextBox Control");
                            return;
                        }
                    }
                    else if (animType == ANIM_TEXTFORMAT)
                    {
                        // try to cast, will throw
                        try
                        {
                            ctrl.txt = (MyGUI::TextBox*)w; // w->getSubWidgetMain()->castType<MyGUI::TextBox>();
                        }
                        catch (...)
                        {
                            LOG("Dashboard (" + filename + "/" + name + "): Lamp controls must use the ImageBox Control");
                            return;
                        }

                        // Prepare for eliminating negative zero (-0.0) display
                        // Must be done on string-level because -0.001 with format "%1.0f" would still produce "-0"
                        if (std::strlen(ctrl.graphicalAnimation.format))
                        {
                            std::snprintf(ctrl.graphicalAnimation.format_neg_zero, 255, ctrl.graphicalAnimation.format, -0.f);
                        }
                    }
                    else if (animType == ANIM_TEXTSTRING)
                    {
                        // try to cast, will throw
                        try
                        {
                            ctrl.txt = (MyGUI::TextBox*)w; // w->getSubWidgetMain()->castType<MyGUI::TextBox>();
                        }
                        catch (...)
                        {
                            LOG("Dashboard (" + filename + "/" + name + "): Lamp controls must use the ImageBox Control");
                            return;
                        }
                    }
                    else if (animType == ANIM_LAMP)
                    {
                        // try to cast, will throw
                        /*
                        {
                            try
                            {
                                w->getSubWidgetMain()->castType<MyGUI::ImageBox>();
                            }
                            catch (...)
                            {
                                LOG("Dashboard ("+filename+"/"+name+"): Lamp controls must use the ImageBox Control");
                                continue;
                            }
                        }
                        */
                        ctrl.img = (MyGUI::ImageBox*)w; //w->getSubWidgetMain()->castType<MyGUI::ImageBox>();
                        if (!ctrl.img)
                        {
                            LOG("Dashboard (" + filename + "/" + name + "): error loading Lamp control");
                            return;
                        }
                    }
                }
                else if (geometricAnimation)
                {
                    if (ctrl.geometricAnimationCount == DD_MAX_GEOMETRIC_ANIMATIONS)
                    {
                        LOG("Dashboard (" + filename + "/" + name + "): Maximum amount of geometric animations reached (" + TOSTRING(DD_MAX_GEOMETRIC_ANIMATIONS) + "). Ignoring the rest.");
                        return;
                    }
                    else
                    {
                        layoutGeometricAnimation_t& geometricAnim = ctrl.geometricAnimations[ctrl.geometricAnimationCount];
                        geometricAnim.linkID = linkID;
                        geometricAnim.animationType = animType;

                        ctrl.geometricAnimationCount++;

                        // parse more attributes
                        geometricAnim.wmin = StringConverter::parseReal(w->getUserString(minNumStr));
                        geometricAnim.wmax = StringConverter::parseReal(w->getUserString(maxNumStr));
                        geometricAnim.vmin = StringConverter::parseReal(w->getUserString(vminNumStr));
                        geometricAnim.vmax = StringConverter::parseReal(w->getUserString(vmaxNumStr));

                        String direction = w->getUserString(directionNumStr);
                        if (direction == "right")
                            geometricAnim.direction = DIRECTION_RIGHT;
                        else if (direction == "left")
                            geometricAnim.direction = DIRECTION_LEFT;
                        else if (direction == "down")
                            geometricAnim.direction = DIRECTION_DOWN;
                        else if (direction == "up")
                            geometricAnim.direction = DIRECTION_UP;
                        else if (!direction.empty())
                        {
                            LOG("Dashboard (" + filename + "/" + name + "): unknown direction: " + direction);
                            return;
                        }

                        // then specializations
                        if (animType == ANIM_ROTATE)
                        {
                            // check if its the correct control
                            // try to cast, will throw
                            // and if the link is a float
                            /*
                            if (manager->getDataType(ctrl.linkID) != DC_FLOAT)
                            {
                                LOG("Dashboard ("+filename+"/"+name+"): Rotating controls can only link to floats");
                                continue;
                            }
                            */

                            try
                            {
                                ctrl.rotImg = w->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
                            }
                            catch (...)
                            {
                                LOG("Dashboard (" + filename + "/" + name + "): Rotating controls must use the RotatingSkin");
                                return;
                            }
                            if (!ctrl.rotImg)
                            {
                                LOG("Dashboard (" + filename + "/" + name + "): error loading rotation control");
                                return;
                            }

                            // special: set rotation center now into the middle
                            ctrl.rotImg->setCenter(MyGUI::IntPoint(w->getWidth() * 0.5f, w->getHeight() * 0.5f));
                        }
                        else if (animType == ANIM_SCALE)
                        {
                            if (geometricAnim.direction == DIRECTION_NONE)
                            {
                                LOG("Dashboard (" + filename + "/" + name + "): direction empty: scale needs a direction");
                                return;
                            }
                        }
                        else if (animType == ANIM_TRANSLATE)
                        {
                            if (geometricAnim.direction == DIRECTION_NONE)
                            {
                                LOG("Dashboard (" + filename + "/" + name + "): direction empty: translate needs a direction");
                                return;
                            }
                        }
                    }
                }
            }

            animNum++;
            animNumStr = fmt::format("anim{}", animNum);
            linkNumStr = fmt::format("link{}", animNum);
            minNumStr = fmt::format("min{}", animNum);
            maxNumStr = fmt::format("max{}", animNum);
            vminNumStr = fmt::format("vmin{}", animNum);
            vmaxNumStr = fmt::format("vmax{}", animNum);
            textureNumStr = fmt::format("texture{}", animNum);
            formatNumStr = fmt::format("format{}", animNum);
            directionNumStr = fmt::format("direction{}", animNum);
            anim = w->getUserString(animNumStr);
        }

        controls[free_controls] = ctrl;
        free_controls++;
        if (free_controls >= MAX_CONTROLS)
        {
            LOG("maximum amount of controls reached, discarding the rest: " + TOSTRING(MAX_CONTROLS));
            return;
        }
    }
    else
    {
        // Control with no animations. Process normal link.
        linkArgs = w->getUserString("link");
        // links for this control?
        if (!linkArgs.empty())
        {
            bool linkParsed = parseLink(linkArgs, linkID, condition, conditionArgument, filename, name);
            if (!linkParsed)
                return;

            ctrl.linkIDForVisibility = linkID;
            ctrl.widget = w;
            ctrl.initialSize = w->getSize();
            ctrl.initialPosition = w->getPosition();
            ctrl.lastState = false;

            controls[free_controls] = ctrl;
            free_controls++;
            if (free_controls >= MAX_CONTROLS)
            {
                LOG("maximum amount of controls reached, discarding the rest: " + TOSTRING(MAX_CONTROLS));
                return;
            }
        }
    }

    // walk the children now
    MyGUI::EnumeratorWidgetPtr e = w->getEnumerator();
    while (e.next())
    {
        loadLayoutRecursive(e.current());
    }
}

void DashBoard::loadLayoutInternal()
{
    widgets = MyGUI::LayoutManager::getInstance().loadLayout(filename, prefix, nullptr); // never has a parent

    for (MyGUI::VectorWidgetPtr::iterator iter = widgets.begin(); iter != widgets.end(); ++iter)
    {
        loadLayoutRecursive(*iter);
    }

    // if this thing should be rendered to texture, relocate the main window to the RTT layer
    if (getIsTextureLayer() && mainWidget)
        mainWidget->detachFromWidget(rttLayer);
}

void DashBoard::setVisible(bool v, bool smooth)
{
    visible = v;

    if (!mainWidget)
    {
        for (MyGUI::VectorWidgetPtr::iterator iter = widgets.begin(); iter != widgets.end(); ++iter)
        {
            (*iter)->setVisible(v);
        }
        return;
    }

    /*
    // buggy for some reason
    if (smooth)
        mainWidget->setVisibleSmooth(v);
    */
    mainWidget->setVisible(v);
}
