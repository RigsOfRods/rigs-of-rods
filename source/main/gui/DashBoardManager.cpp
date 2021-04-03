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

#include "Application.h"
#include "Utils.h"

#include <OgreOverlayManager.h>
#include <OgreOverlayContainer.h>

#include <fmt/format.h>

using namespace Ogre;
using namespace RoR;

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

DashBoard::DashBoard(DashBoardManager* manager, Ogre::String filename, bool _textureLayer) : manager(manager), filename(filename), free_controls(0), visible(false), textureLayer(_textureLayer)
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
    for (int i = 0; i < free_controls; i++)
    {
        bool enabled = manager->getEnabled(controls[i].linkID);

        controls[i].widget->setVisible(enabled);
    }
}

void DashBoard::update(float& dt)
{
    // walk all controls and animate them
    for (int i = 0; i < free_controls; i++)
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
            if (state)
            {
                controls[i].img->setImageTexture(String(controls[i].texture) + "-on.png");
            }
            else
            {
                controls[i].img->setImageTexture(String(controls[i].texture) + "-off.png");
            }
        }
        else if (controls[i].animationType == ANIM_SERIES)
        {
            float val = manager->getNumeric(controls[i].linkID);

            String fn = String(controls[i].texture) + String("-") + TOSTRING((int)val) + String(".png");

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            controls[i].img->setImageTexture(fn);
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
            char* val = manager->getChar(controls[i].linkID);
            controls[i].txt->setCaption(MyGUI::UString(val));
        }
    }
}

void DashBoard::windowResized()
{

}

#if 0
void DashBoard::loadLayoutRecursive(Ogre::OverlayContainer* container)
{
    std::string name = w->getName();
    std::string anim = w->getUserString("anim");
    std::string debug = w->getUserString("debug");
    std::string linkArgs = w->getUserString("link");

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

    // animations for this control?
    if (!linkArgs.empty())
    {
        layoutLink_t ctrl;
        memset(&ctrl, 0, sizeof(ctrl));

        if (!name.empty())
            strncpy(ctrl.name, name.c_str(), 255);
        ctrl.widget = w;
        ctrl.initialSize = w->getSize();
        ctrl.initialPosition = w->getPosition();
        ctrl.last = 1337.1337f; // force update
        ctrl.lastState = true;

        // establish the link
        {
            replaceString(linkArgs, "&gt;", ">");
            replaceString(linkArgs, "&lt;", "<");
            String linkName = "";
            if (linkArgs.empty())
            {
                LOG("Dashboard ("+filename+"/"+name+"): empty Link");
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
                    LOG("Dashboard ("+filename+"/"+name+"): error in conditional Link: " + linkArgs);
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
                    LOG("Dashboard ("+filename+"/"+name+"): error in conditional Link: " + linkArgs);
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
                LOG("Dashboard ("+filename+"/"+name+"): unknown Link: " + linkName);
                return;
            }

            ctrl.linkID = linkID;
        }

        // parse more attributes
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
            LOG("Dashboard ("+filename+"/"+name+"): unknown direction: " + direction);
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
                LOG("Dashboard ("+filename+"/"+name+"): Rotating controls must use the RotatingSkin");
                return;
            }
            if (!ctrl.rotImg)
            {
                LOG("Dashboard ("+filename+"/"+name+"): error loading rotation control");
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
                LOG("Dashboard ("+filename+"/"+name+"): direction empty: scale needs a direction");
                return;
            }
        }
        else if (anim == "translate")
        {
            ctrl.animationType = ANIM_TRANSLATE;
            if (ctrl.direction == DIRECTION_NONE)
            {
                LOG("Dashboard ("+filename+"/"+name+"): direction empty: translate needs a direction");
                return;
            }
        }
        else if (anim == "series")
        {
            ctrl.animationType = ANIM_SERIES;
            ctrl.img = (MyGUI::ImageBox *)w; //w->getSubWidgetMain()->castType<MyGUI::ImageBox>();
            if (!ctrl.img)
            {
                LOG("Dashboard ("+filename+"/"+name+"): error loading series control");
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
                LOG("Dashboard ("+filename+"/"+name+"): textcolor controls must use the TextBox Control");
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
                LOG("Dashboard ("+filename+"/"+name+"): Lamp controls must use the ImageBox Control");
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
        else if (anim == "textstring")
        {
            // try to cast, will throw
            try
            {
                ctrl.txt = (MyGUI::TextBox *)w; // w->getSubWidgetMain()->castType<MyGUI::TextBox>();
            }
            catch (...)
            {
                LOG("Dashboard ("+filename+"/"+name+"): Lamp controls must use the ImageBox Control");
                return;
            }
            ctrl.animationType = ANIM_TEXTSTRING;
        }
        else if (anim == "lamp")
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
            ctrl.animationType = ANIM_LAMP;
            ctrl.img = (MyGUI::ImageBox *)w; //w->getSubWidgetMain()->castType<MyGUI::ImageBox>();
            if (!ctrl.img)
            {
                LOG("Dashboard ("+filename+"/"+name+"): error loading Lamp control");
                return;
            }
        }

        controls[free_controls] = ctrl;
        free_controls++;
        if (free_controls >= MAX_CONTROLS)
        {
            LOG("maximum amount of controls reached, discarding the rest: " + TOSTRING(MAX_CONTROLS));
            return;
        }
    }

    // walk the children now
    MyGUI::EnumeratorWidgetPtr e = w->getEnumerator();
    while (e.next())
    {
        loadLayoutRecursive(e.current());
    }
}
#endif
void DashBoard::loadLayout(Ogre::String filename)
{
    m_overlay = Ogre::OverlayManager::getSingleton().getByName(filename);
    if (m_overlay)
    {
        const Overlay::OverlayContainerList& list = m_overlay->get2DElements();
        for (OverlayContainer* container: list)
        {
            OverlayContainer::ChildIterator child_iterator = container->getChildIterator();
            for (Ogre::OverlayContainer::ChildMap::value_type& child: child_iterator)
            {
                LOG(fmt::format(
                    "DashBoard::loadLayout() - iterating child '{}' of type '{}'",
                    child.second->getName(), child.second->getTypeName()));

                if (child.second->getTypeName() == TextAreaDashboardIndicator::OVERLAY_ELEMENT_TYPE_NAME)
                {
                    TextAreaDashboardIndicator* di = static_cast<TextAreaDashboardIndicator*>(child.second);
                    LOG(fmt::format(
                        "DashBoard::loadLayout() - got indicator with link '{}' and anim '{}'",
                        di->getLinkStr(), di->getAnimStr()));
                }
            }

            //this->loadLayoutRecursive(container);
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

// @@@@@@@@@@@@@@@@@@@@@@@@@ PROTOTYPE @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// ------ base dash element -------

BaseDashboardIndicator::CmdAnim TextAreaDashboardIndicator::ms_anim_cmd;
BaseDashboardIndicator::CmdLink TextAreaDashboardIndicator::ms_link_cmd;
/*
String BaseDashboardIndicator::CmdAnim::doGet( const void* target ) const
{
    return static_cast<const BaseDashboardIndicator*>(target)->getAnimStr();
}
String BaseDashboardIndicator::CmdLink::doGet( const void* target ) const
{
    return static_cast<const BaseDashboardIndicator*>(target)->getLinkStr();
}

void BaseDashboardIndicator::CmdAnim::doSet( void* target, const String& val )
{
    BaseDashboardIndicator* obj = static_cast<BaseDashboardIndicator*>(target);
    obj->setAnimStr(val);
}
void BaseDashboardIndicator::CmdLink::doSet( void* target, const String& val )
{
    BaseDashboardIndicator* obj = static_cast<BaseDashboardIndicator*>(target);
    obj->setLinkStr(val);
}
*/

void BaseDashboardIndicator::addExtensionParams(Ogre::ParamDictionary* dict)
{
    // OGRE overlay param extension mechanism, step 3
    // See commentary on class BaseDashboardIndicator

    dict->addParameter(ParameterDef("ror_anim", 
        "How to animate this dashboard control."
        , PT_STRING),
        &ms_anim_cmd);

    dict->addParameter(ParameterDef("ror_link", 
        "What gameplay data to display with this dashboard control."
        , PT_STRING),
        &ms_link_cmd);
}

// ----- textarea

const Ogre::String TextAreaDashboardIndicator::OVERLAY_ELEMENT_TYPE_NAME("TextAreaDashboardIndicator");

TextAreaDashboardIndicator::TextAreaDashboardIndicator(const String& name)
    : TextAreaOverlayElement(name)
{
    // Register this class in OGRE parameter system, see Ogre::StringInterface.
    this->createParamDictionary(OVERLAY_ELEMENT_TYPE_NAME);
    // Add Ogre::TextAreaOverlayElement (and ancestor classes) parameters.
    this->addBaseParameters();
    // Add RoR::BaseDashboardIndicator parameters.
    this->addExtensionParams(this->getParamDictionary());
}

String TextAreaDashboardIndicator::CmdAnim::doGet( const void* target ) const
{
    return static_cast<const TextAreaDashboardIndicator*>(target)->getAnimStr();
}
String TextAreaDashboardIndicator::CmdLink::doGet( const void* target ) const
{
    return static_cast<const TextAreaDashboardIndicator*>(target)->getLinkStr();
}

void TextAreaDashboardIndicator::CmdAnim::doSet( void* target, const String& val )
{
    TextAreaDashboardIndicator* obj = static_cast<TextAreaDashboardIndicator*>(target);
    obj->setAnimStr(val);
}
void TextAreaDashboardIndicator::CmdLink::doSet( void* target, const String& val )
{
    TextAreaDashboardIndicator* obj = static_cast<TextAreaDashboardIndicator*>(target);
    obj->setLinkStr(val);
}

// ------ factory

Ogre::OverlayElement* TextAreaDashboardIndicatorFactory::createOverlayElement(const Ogre::String& instanceName)
{
    return OGRE_NEW TextAreaDashboardIndicator(instanceName);
}

const Ogre::String& TextAreaDashboardIndicatorFactory::getTypeName() const
{
    return TextAreaDashboardIndicator::OVERLAY_ELEMENT_TYPE_NAME;
}

