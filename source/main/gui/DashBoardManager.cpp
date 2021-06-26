/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

#include "Application.h"
#include "Console.h"
#include "DashPanelOverlayElement.h"
#include "DashTextAreaOverlayElement.h"
#include "Utils.h"

#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>

#include <fmt/format.h>

using namespace Ogre;
using namespace RoR;

                     /////////////////////////////
    // ============= 'OVERDASH' REMAKE IN PROGRESS =================
    //   MyGUI widgets and layout XML files are being replaced by Ogre Overlays.
    //   Also, LOG() function is being replaced with console messages.
    // =============================================================
                     /////////////////////////////

#define INITDATA(key, type, name) data[key] = dashData_t(type, name)

DashBoardManager::DashBoardManager(void) : visible(true), free_dashboard(0)
{
    // clear some things
    memset(&dashboards, 0, sizeof(dashboards));

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
    INITDATA(DD_LIGHTS                  , DC_BOOL , "lights");
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
    INITDATA(DD_EDITOR_NODE_INFO        , DC_CHAR , "editor_node_info");

    INITDATA(DD_ODOMETER_TOTAL          , DC_FLOAT, "odometer_total");
    INITDATA(DD_ODOMETER_USER           , DC_FLOAT, "odometer_user");

    INITDATA(DD_SIGNAL_TURNLEFT         , DC_BOOL, "signal_turnleft");
    INITDATA(DD_SIGNAL_TURNRIGHT        , DC_BOOL, "signal_turnright");
    INITDATA(DD_SIGNAL_WARNING          , DC_BOOL, "signal_warning");
    // load dash fonts
    MyGUI::ResourceManager::getInstance().load("MyGUI_FontsDash.xml");
}

DashBoardManager::~DashBoardManager(void)
{
    // free all objects
    for (int i = 0; i < free_dashboard; i++)
    {
        if (!dashboards[i])
            continue;

        delete(dashboards[i]);
        dashboards[i] = 0;
    }
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

int DashBoardManager::loadDashBoard(Ogre::String filename, bool textureLayer)
{
    if (free_dashboard >= MAX_DASH)
    {
        LOG("maximum amount of dashboards per truck reached, discarding the rest: " + TOSTRING(MAX_DASH));
        return 1;
    }

    DashBoard* d = new DashBoard(this, filename, textureLayer);
    d->setVisible(true);

    dashboards[free_dashboard] = d;
    free_dashboard++;

    return 0;
}

void DashBoardManager::update(float& dt)
{
    // TODO: improve logic: only update visible dashboards
    for (int i = 0; i < free_dashboard; i++)
    {
        dashboards[i]->update(dt);
    }
}

void DashBoardManager::updateFeatures()
{
    for (int i = 0; i < free_dashboard; i++)
    {
        dashboards[i]->updateFeatures();
    }
}

float DashBoardManager::getNumeric(size_t key)
{
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

std::string DashBoardManager::getString(size_t key)
{
    if (data[key].type == DC_CHAR)
        return data[key].data.value_char;
    else
        return fmt::format("{}", this->getNumeric(key));
}

void DashBoardManager::setVisible(bool visibility)
{
    visible = visibility;
    for (int i = 0; i < free_dashboard; i++)
    {
        if (!dashboards[i]->getIsTextureLayer())
            dashboards[i]->setVisible(visibility);
    }
}

void DashBoardManager::setVisible3d(bool visibility)
{

}

void DashBoardManager::windowResized()
{
    for (int i = 0; i < free_dashboard; i++)
    {
        if (dashboards[i])
            dashboards[i]->windowResized();
    }
}

// DASHBOARD class below

DashBoard::DashBoard(DashBoardManager* manager, Ogre::String filename, bool _textureLayer) : manager(manager), filename(filename), visible(false), textureLayer(_textureLayer)
{
    // use 'this' class pointer to make layout unique
    prefix = MyGUI::utility::toString(this, "_");
    memset(&controls, 0, sizeof(controls));
    loadLayout(filename);
    // hide first
    if (m_overlay)
        m_overlay->hide();
}

DashBoard::~DashBoard()
{
    Ogre::OverlayManager::getSingleton().destroy(m_overlay);
    m_overlay = nullptr;
}

void DashBoard::updateFeatures()
{
    // this hides / shows parts of the gui depending on the vehicle features
    for (int i = 0; i < (int)controls.size(); i++)
    {
        bool enabled = manager->getEnabled(controls[i].linkID);

        if (enabled)
            controls[i].element->show();
        else
            controls[i].element->hide();
    }
}

void DashBoard::update(float& dt)
{
    // walk all controls and animate them
    for (int i = 0; i < (int)controls.size(); i++)
    {
        // get its value from its linkage
        if (controls[i].animationType == ANIM_ROTATE)
        {
            // get the value
            float val = manager->getNumeric(controls[i].linkID);
            // calculate the angle
            float angle = (val - controls[i].vmin) * (controls[i].wmax - controls[i].wmin) / (controls[i].vmax - controls[i].vmin) + controls[i].wmin;

            if (fabs(val - controls[i].last) < 0.02f)
                continue;

            controls[i].last = val;

            // enforce limits
            if (angle < controls[i].wmin)
                angle = controls[i].wmin;
            else if (angle > controls[i].wmax)
                angle = controls[i].wmax;
            // rotate finally
            controls[i].rotImg->setAngle(Ogre::Degree(angle).valueRadians());
        }
        else if (controls[i].animationType == ANIM_LAMP)
        {
            // or a lamp?
            bool state = false;
            // conditional
            if (controls[i].condition == CONDITION_GREATER)
            {
                float val = manager->getNumeric(controls[i].linkID);
                state = (val > controls[i].conditionArgument);
            }
            else if (controls[i].condition == CONDITION_LESSER)
            {
                float val = manager->getNumeric(controls[i].linkID);
                state = (val < controls[i].conditionArgument);
            }
            else
            {
                state = (manager->getNumeric(controls[i].linkID) > 0);
            }

            if (state == controls[i].lastState)
                continue;
            controls[i].lastState = state;

            // switch states
            DashPanelOverlayElement* lamp = static_cast<DashPanelOverlayElement*>(controls[i].element);
            lamp->setMaterial(state ? controls[i].materials[1] : controls[i].materials[0]);
        }
        else if (controls[i].animationType == ANIM_SERIES)
        {
            float val = manager->getNumeric(controls[i].linkID);

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            // switch states
            if ((int)val < 0 || (int)val >= (int)controls[i].materials.size())
            {
                continue;
            }
            DashPanelOverlayElement* lamp = static_cast<DashPanelOverlayElement*>(controls[i].element);
            lamp->setMaterial(controls[i].materials[(int)val]);
        }
        else if (controls[i].animationType == ANIM_SCALE)
        {
            float val = manager->getNumeric(controls[i].linkID);

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            float scale = (val - controls[i].vmin) * (controls[i].wmax - controls[i].wmin) / (controls[i].vmax - controls[i].vmin) + controls[i].wmin;
            if (controls[i].direction == DIRECTION_UP)
            {
                controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top - scale);
                controls[i].widget->setSize(controls[i].initialSize.width, controls[i].initialSize.height + scale);
            }
            else if (controls[i].direction == DIRECTION_DOWN)
            {
                controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top);
                controls[i].widget->setSize(controls[i].initialSize.width, controls[i].initialSize.height + scale);
            }
            else if (controls[i].direction == DIRECTION_LEFT)
            {
                controls[i].widget->setPosition(controls[i].initialPosition.left - scale, controls[i].initialPosition.top);
                controls[i].widget->setSize(controls[i].initialSize.width + scale, controls[i].initialSize.height);
            }
            else if (controls[i].direction == DIRECTION_RIGHT)
            {
                controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top);
                controls[i].widget->setSize(controls[i].initialSize.width + scale, controls[i].initialSize.height);
            }
        }
        else if (controls[i].animationType == ANIM_TRANSLATE)
        {
            float val = manager->getNumeric(controls[i].linkID);

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            float translation = (val - controls[i].vmin) * (controls[i].wmax - controls[i].wmin) / (controls[i].vmax - controls[i].vmin) + controls[i].wmin;
            if (controls[i].direction == DIRECTION_UP)
                controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top - translation);
            else if (controls[i].direction == DIRECTION_DOWN)
                controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top + translation);
            else if (controls[i].direction == DIRECTION_LEFT)
                controls[i].widget->setPosition(controls[i].initialPosition.left - translation, controls[i].initialPosition.top);
            else if (controls[i].direction == DIRECTION_RIGHT)
                controls[i].widget->setPosition(controls[i].initialPosition.left + translation, controls[i].initialPosition.top);
        }
        else if (controls[i].animationType == ANIM_TEXTFORMAT)
        {
            float val = manager->getNumeric(controls[i].linkID);

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            MyGUI::UString s;
            if (strlen(controls[i].format) == 0)
            {
                s = Ogre::StringConverter::toString(val);
            }
            else
            {
                char tmp[1024] = "";
                sprintf(tmp, controls[i].format, val);

                // Detect and eliminate negative zero (-0) on output
                if (strcmp(tmp, controls[i].format_neg_zero) == 0)
                {
                    sprintf(tmp, controls[i].format, 0.f);
                }

                s = MyGUI::UString(tmp);
            }

            controls[i].txt->setCaption(s);
        }
        else if (controls[i].animationType == ANIM_TEXTSTRING)
        {
            controls[i].element->setCaption(
                manager->getString(controls[i].linkID));
        }
    }
}

void DashBoard::windowResized()
{

}

bool DashBoard::setupLampAnim(layoutLink_t& ctrl)
{
    /// Materials must have suffix "-on" and "-off". One must be specified in overlay script - the other will be deduced.

    DashPanelOverlayElement* elem = static_cast<DashPanelOverlayElement*>(ctrl.element);

    if (!elem->getMaterial())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("No material assigned - element '{}' will not be animated", elem->getName()));
        return false;
    }

    ctrl.animationType = ANIM_LAMP;
    ctrl.materials.resize(2);

    // Fetch materials
    if (Ogre::StringUtil::endsWith(elem->getMaterial()->getName(), "-on"))
    {
        ctrl.materials[1] = elem->getMaterial();
        Ogre::String base_name = elem->getMaterial()->getName().substr(0, elem->getMaterial()->getName().length() - 3);
        ctrl.materials[0] = Ogre::MaterialManager::getSingleton().getByName(base_name + "-off"); //FIXME: resource group!!
    }
    else if (Ogre::StringUtil::endsWith(elem->getMaterial()->getName(), "-off"))
    {
        ctrl.materials[0] = elem->getMaterial();
        Ogre::String base_name = elem->getMaterial()->getName().substr(0, elem->getMaterial()->getName().length() - 4);
        ctrl.materials[1] = Ogre::MaterialManager::getSingleton().getByName(base_name + "-on"); //FIXME: resource group!!
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Unrecognized material '{}' - element '{}' will not be animated",
                elem->getMaterial()->getName(), elem->getName()));
        return false;
    }
    return true;
}

bool DashBoard::setupSeriesAnim(layoutLink_t& ctrl)
{
    /// Materials must end by integer. One must be specified in overlay script - the other will be deduced.

    DashPanelOverlayElement* elem = static_cast<DashPanelOverlayElement*>(ctrl.element);

    // No material? Return error.
    if (!elem->getMaterial())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("No material assigned - element '{}' will not be animated", elem->getName()));
        return false;
    }

    // Analyze material
    size_t num_start = elem->getMaterial()->getName().length();
    while (num_start > 0 && isdigit(elem->getMaterial()->getName()[num_start - 1]))
    {
        num_start--;
    }

    if (num_start == elem->getMaterial()->getName().length())
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Material '{}' missing number - element '{}' will not be animated",
                elem->getMaterial()->getName(), elem->getName()));
        return false;
    }

    ctrl.animationType = ANIM_SERIES;

    // Find all available materials, starting with 0, ending when a number is not found.
    Ogre::String series_base_name = elem->getMaterial()->getName().substr(0, num_start);
    bool keep_searching = true;
    int find_num = 0;
    while (keep_searching)
    {
        Ogre::String mat_name = fmt::format("{}{}", series_base_name, find_num);
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(mat_name);//FIXME: resource group!!
        if (mat)
        {
            ctrl.materials.push_back(mat);
            find_num++;
        }
        else
        {
            keep_searching = false;
        }
    }

    // Check we have at least 2 materials
    if (ctrl.materials.size() < 2)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Only 1 material found '{}' - element '{}' will not be animated",
                elem->getMaterial()->getName(), elem->getName()));
        return false;
    }

    return true;
}

void DashBoard::setupElement(Ogre::OverlayElement* elem)
{
    // retrieve params
    String anim, linkArgs;
    if (elem->getTypeName() == DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        DashTextAreaOverlayElement* dta = static_cast<DashTextAreaOverlayElement*>(elem);
        anim = dta->getAnimStr();
        linkArgs = dta->getLinkStr();
    }
    else if (elem->getTypeName() == DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
    {
        DashPanelOverlayElement* dash_elem = static_cast<DashPanelOverlayElement*>(elem);
        anim = dash_elem->getAnimStr();
        linkArgs = dash_elem->getLinkStr();
    }
    else
    {
        return; // Unsupported element
    }

    LOG(fmt::format(
        "DashBoard::setupElement() - processing '{}' (type: '{}') with anim '{}' and link '{}'",
        elem->getName(), elem->getTypeName(), anim, linkArgs));

    // animations for this control?
    if (!linkArgs.empty())
    {
        layoutLink_t ctrl;
        ctrl.element = elem;
        #if 0 // OVERDASH
        ctrl.initialSize = w->getSize();
        ctrl.initialPosition = w->getPosition();
        #endif // OVERDASH

        // establish the link
        {
            String linkName = "";
            if (linkArgs.empty())
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): empty Link");
                return;
            }
            // conditional checks
            // TODO: improve the logic, this is crap ...
            if (linkArgs.find(">") != linkArgs.npos)
            {
                Ogre::StringVector args = Ogre::StringUtil::split(linkArgs, ">");
                if (args.size() == 2)
                {
                    linkName = args[0];
                    ctrl.conditionArgument = StringConverter::parseReal(args[1]);
                    ctrl.condition = CONDITION_GREATER;
                }
                else
                {
                    LOG("Dashboard ("+filename+"/"+elem->getName()+"): error in conditional Link: " + linkArgs);
                    return;
                }
            }
            else if (linkArgs.find("<") != linkArgs.npos)
            {
                Ogre::StringVector args = Ogre::StringUtil::split(linkArgs, "<");
                if (args.size() == 2)
                {
                    linkName = args[0];
                    ctrl.conditionArgument = StringConverter::parseReal(args[1]);
                    ctrl.condition = CONDITION_LESSER;
                }
                else
                {
                    LOG("Dashboard ("+filename+"/"+elem->getName()+"): error in conditional Link: " + linkArgs);
                    return;
                }
            }
            else
            {
                ctrl.condition = CONDITION_NONE;
                ctrl.conditionArgument = 0;
                linkName = linkArgs;
            }

            // now try to get the enum id for it
            int linkID = manager->getLinkIDForName(linkName);
            if (linkID < 0)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): unknown Link: " + linkName);
                return;
            }

            ctrl.linkID = linkID;
        }

        // parse more attributes
#if 0 // OVERDASH
        ctrl.wmin = StringConverter::parseReal(w->getUserString("min"));
        ctrl.wmax = StringConverter::parseReal(w->getUserString("max"));
        ctrl.vmin = StringConverter::parseReal(w->getUserString("vmin"));
        ctrl.vmax = StringConverter::parseReal(w->getUserString("vmax"));

        String texture = w->getUserString("texture");
        if (!texture.empty())
            strncpy(ctrl.texture, texture.c_str(), 255);

        String format = w->getUserString("format");
        if (!format.empty())
            strncpy(ctrl.format, format.c_str(), 255);

        String direction = w->getUserString("direction");
        if (direction == "right")
            ctrl.direction = DIRECTION_RIGHT;
        else if (direction == "left")
            ctrl.direction = DIRECTION_LEFT;
        else if (direction == "down")
            ctrl.direction = DIRECTION_DOWN;
        else if (direction == "up")
            ctrl.direction = DIRECTION_UP;
        else if (!direction.empty())
        {
            LOG("Dashboard ("+filename+"/"+elem->getName()+"): unknown direction: " + direction);
            return;
        }


        // then specializations
        if (anim == "rotate")
        {
            ctrl.animationType = ANIM_ROTATE;
            // check if its the correct control
            // try to cast, will throw
            // and if the link is a float
            /*
            if (manager->getDataType(ctrl.linkID) != DC_FLOAT)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): Rotating controls can only link to floats");
                continue;
            }
            */

            try
            {
                ctrl.rotImg = w->getSubWidgetMain()->castType<MyGUI::RotatingSkin>();
            }
            catch (...)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): Rotating controls must use the RotatingSkin");
                return;
            }
            if (!ctrl.rotImg)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): error loading rotation control");
                return;
            }

            // special: set rotation center now into the middle
            ctrl.rotImg->setCenter(MyGUI::IntPoint(w->getHeight() * 0.5f, w->getWidth() * 0.5f));
        }
        else if (anim == "scale")
        {
            ctrl.animationType = ANIM_SCALE;
            if (ctrl.direction == DIRECTION_NONE)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): direction empty: scale needs a direction");
                return;
            }
        }
        else if (anim == "translate")
        {
            ctrl.animationType = ANIM_TRANSLATE;
            if (ctrl.direction == DIRECTION_NONE)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): direction empty: translate needs a direction");
                return;
            }
        }
        else if (anim == "series")
        {
            ctrl.animationType = ANIM_SERIES;
            ctrl.img = (MyGUI::ImageBox *)w; //w->getSubWidgetMain()->castType<MyGUI::ImageBox>();
            if (!ctrl.img)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): error loading series control");
                return;
            }
        }
        else if (anim == "textcolor" || anim == "textcolour")
        {
            ctrl.animationType = ANIM_TEXTCOLOR;

            // try to cast, will throw
            try
            {
                ctrl.txt = (MyGUI::TextBox *)w;
            }
            catch (...)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): textcolor controls must use the TextBox Control");
                return;
            }
        }
        else if (anim == "textformat")
        {
            // try to cast, will throw
            try
            {
                ctrl.txt = (MyGUI::TextBox *)w; // w->getSubWidgetMain()->castType<MyGUI::TextBox>();
            }
            catch (...)
            {
                LOG("Dashboard ("+filename+"/"+elem->getName()+"): Lamp controls must use the ImageBox Control");
                return;
            }
            ctrl.animationType = ANIM_TEXTFORMAT;

            // Prepare for eliminating negative zero (-0.0) display
            // Must be done on string-level because -0.001 with format "%1.0f" would still produce "-0"
            if (std::strlen(ctrl.format))
            {
                std::snprintf(ctrl.format_neg_zero, 255, ctrl.format, -0.f);
            }
        }
        else
        #endif // OVERDASH
        if (anim == "textstring")
        {
            if (elem->getTypeName() != DashTextAreaOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
            {
                App::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format(
                        "Dashboard element '{}' will not be animated;"
                        "anim '{}' is not compatible with type '{}'",
                        elem->getName(), anim, elem->getTypeName()));
                return;
            }
            else
            {
                ctrl.animationType = ANIM_TEXTSTRING;
            }
        }
        else if (anim == "lamp")
        {
            if (elem->getTypeName() != DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
            {
                App::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format(
                        "Dashboard element '{}' will not be animated;"
                        "anim '{}' is not compatible with type '{}'",
                        elem->getName(), anim, elem->getTypeName()));
                return;
            }
            else
            {
                if (!this->setupLampAnim(ctrl))
                    return;
            }
        }
        else if (anim == "series")
        {
            if (elem->getTypeName() != DashPanelOverlayElement::OVERLAY_ELEMENT_TYPE_NAME)
            {
                App::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_WARNING,
                    fmt::format(
                        "Dashboard element '{}' will not be animated;"
                        "anim '{}' is not compatible with type '{}'",
                        elem->getName(), anim, elem->getTypeName()));
                return;
            }
            else
            {
                if (!this->setupSeriesAnim(ctrl))
                    return;
            }
        }
        controls.push_back(ctrl);
    }

}

void DashBoard::loadLayout(Ogre::String filename)
{
    m_overlay = Ogre::OverlayManager::getSingleton().getByName(filename);
    if (m_overlay)
    {
        const Overlay::OverlayContainerList& list = m_overlay->get2DElements();
        for (OverlayContainer* container: list)
        {
            LOG(fmt::format(
                "DashBoard::loadLayout() - processing element '{}' of type '{}'",
                container->getName(), container->getTypeName()));
            this->setupElement(container);

            // TODO: recurse!
            OverlayContainer::ChildIterator child_iterator = container->getChildIterator();
            for (Ogre::OverlayContainer::ChildMap::value_type& child: child_iterator)
            {
                LOG(fmt::format(
                    "DashBoard::loadLayout() - iterating child '{}' of type '{}'",
                    child.second->getName(), child.second->getTypeName()));

                this->setupElement(child.second);
            }
        }
    }

}


void DashBoard::setVisible(bool v)
{
    if (v)
        m_overlay->show();
    else
        m_overlay->hide();
}

