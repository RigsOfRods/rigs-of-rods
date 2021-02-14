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
    for (int i = 0; i < free_dashboard; i++)
    {
        if (dashboards[i]->getIsTextureLayer())
            dashboards[i]->setVisible(visibility);
    }
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

    memset(&controls, 0, sizeof(controls));
    

}

DashBoard::~DashBoard()
{

}

void DashBoard::updateFeatures()
{
    // this hides / shows parts of the gui depending on the vehicle features
    for (int i = 0; i < free_controls; i++)
    {
        bool enabled = manager->getEnabled(controls[i].linkID);

        //TODO OGRE2x controls[i].widget->setVisible(enabled);
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
            // TODO OGRE2x // controls[i].rotImg->setAngle(Ogre::Degree(angle).valueRadians());
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
                // TODO OGRE2x // controls[i].img->setImageTexture(String(controls[i].texture) + "-on.png");
            }
            else
            {
                // TODO OGRE2x // controls[i].img->setImageTexture(String(controls[i].texture) + "-off.png");
            }
        }
        else if (controls[i].animationType == ANIM_SERIES)
        {
            float val = manager->getNumeric(controls[i].linkID);

            String fn = String(controls[i].texture) + String("-") + TOSTRING((int)val) + String(".png");

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            // TODO OGRE2x // controls[i].img->setImageTexture(fn);
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
                // TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top - scale);
                // TODO OGRE2x // controls[i].widget->setSize(controls[i].initialSize.width, controls[i].initialSize.height + scale);
            }
            else if (controls[i].direction == DIRECTION_DOWN)
            {
                // TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top);
                // TODO OGRE2x // controls[i].widget->setSize(controls[i].initialSize.width, controls[i].initialSize.height + scale);
            }
            else if (controls[i].direction == DIRECTION_LEFT)
            {
                // TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left - scale, controls[i].initialPosition.top);
                // TODO OGRE2x // controls[i].widget->setSize(controls[i].initialSize.width + scale, controls[i].initialSize.height);
            }
            else if (controls[i].direction == DIRECTION_RIGHT)
            {
                // TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top);
                // TODO OGRE2x // controls[i].widget->setSize(controls[i].initialSize.width + scale, controls[i].initialSize.height);
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
                ;// TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top - translation);
            else if (controls[i].direction == DIRECTION_DOWN)
                ;// TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left, controls[i].initialPosition.top + translation);
            else if (controls[i].direction == DIRECTION_LEFT)
                ;// TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left - translation, controls[i].initialPosition.top);
            else if (controls[i].direction == DIRECTION_RIGHT)
                ;// TODO OGRE2x // controls[i].widget->setPosition(controls[i].initialPosition.left + translation, controls[i].initialPosition.top);
        }
        else if (controls[i].animationType == ANIM_TEXTFORMAT)
        {
            float val = manager->getNumeric(controls[i].linkID);

            if (fabs(val - controls[i].last) < 0.2f)
                continue;
            controls[i].last = val;

            Ogre::v1::DisplayString s;
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

                s = tmp;
            }

            // TODO OGRE2x // controls[i].txt->setCaption(s);
        }
        else if (controls[i].animationType == ANIM_TEXTSTRING)
        {
            char* val = manager->getChar(controls[i].linkID);
            // TODO OGRE2x // controls[i].txt->setCaption(MyGUI::UString(val));
        }
    }
}

void DashBoard::windowResized()
{

}
