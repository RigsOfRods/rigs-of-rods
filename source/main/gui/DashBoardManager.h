/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   19th of October 2011


#pragma once

                     /////////////////////////////
    // ============= 'OVERDASH' REMAKE IN PROGRESS =================
    //   MyGUI widgets and layout XML files are being replaced by Ogre Overlays.
    //   Also, LOG() function is being replaced with console messages.
    // =============================================================
                     /////////////////////////////


#include "Application.h"

#include <MyGUI.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayElementFactory.h>

#include <string>

namespace RoR {

#define DD_MAXCHAR 255
#define DD_MAX_SCREWPROP  6
#define DD_MAX_AEROENGINE 6
#define DD_MAX_WING       6
#define MAX_DASH          6

typedef union dataContainer_t
{
    bool value_bool;
    int value_int;
    float value_float;
    char value_char[DD_MAXCHAR];
} dataContainer_t;

enum
{
    DC_BOOL,
    DC_INT,
    DC_FLOAT,
    DC_CHAR,
    DC_INVALID
};

typedef struct dashData_t
{
    char type; // DC_*
    dataContainer_t data;
    bool enabled;
    const char* name; // char string of name

    dashData_t() : type(DC_INVALID), name("")
    {
        memset(&data, 0, sizeof(data));
        enabled = false;
    }

    dashData_t(char type, const char* name) : type(type), name(name)
    {
        memset(&data, 0, sizeof(data));
        enabled = true;
    }
} dashData_t;

// DashData enum definition
// important: if you add things here, also change the initialization in the constructor
enum
{
    DD_ENGINE_RPM,
    DD_ENGINE_SPEEDO_KPH,      /// speedo in kilometer per hour
    DD_ENGINE_SPEEDO_MPH,      /// speedo in miles per hour
    DD_ENGINE_TURBO,           /// turbo gauge
    DD_ENGINE_IGNITION,
    DD_ENGINE_BATTERY,         /// battery lamp
    DD_ENGINE_CLUTCH_WARNING,  /// clutch warning lamp

    DD_ENGINE_GEAR,            /// current gear
    DD_ENGINE_NUM_GEAR,
    DD_ENGINE_GEAR_STRING,     /// string like "<current gear>/<max gear>"
    DD_ENGINE_AUTOGEAR_STRING, /// string like "P R N G"
    DD_ENGINE_AUTO_GEAR,       /// automatic gear

    DD_ENGINE_CLUTCH, // the engines clutch

    DD_BRAKE, // the brake application in % 0-1
    DD_ACCELERATOR, // accelerator pedal in %, 0-1

    DD_ROLL, // roll of the chassis
    DD_ROLL_CORR, // correction roll of the chassis
    DD_ROLL_CORR_ACTIVE, // correction rolling active

    DD_PITCH, /// chassis pitch

    DD_PARKINGBRAKE, /// parking brake status
    DD_LOCKED, /// locked lamp

    DD_LOW_PRESSURE, /// low pressure
    DD_LIGHTS, /// lights on

    DD_TRACTIONCONTROL_MODE,
    DD_ANTILOCKBRAKE_MODE,
    DD_TIES_MODE, /// ties locked

    // water things
    DD_SCREW_THROTTLE_0, // throttle for screwprop 0
    DD_SCREW_THROTTLE_1,
    DD_SCREW_THROTTLE_2,
    DD_SCREW_THROTTLE_3,
    DD_SCREW_THROTTLE_4,
    DD_SCREW_THROTTLE_5, // if you add some, change DD_MAX_SCREWPROP and DD_SCREW_STEER_*

    DD_SCREW_STEER_0, // steering direction of screw 0
    DD_SCREW_STEER_1, // steering direction of screw 1
    DD_SCREW_STEER_2, // steering direction of screw 2
    DD_SCREW_STEER_3, // steering direction of screw 3
    DD_SCREW_STEER_4, // steering direction of screw 4
    DD_SCREW_STEER_5, // steering direction of screw 4,  if you add some, change DD_MAX_SCREWPROP and DD_SCREW_THROTTLE_*

    DD_WATER_DEPTH, // how much water is under the boat
    DD_WATER_SPEED, // speed in knots

    // airplane things
    DD_AEROENGINE_THROTTLE_0,
    DD_AEROENGINE_THROTTLE_1,
    DD_AEROENGINE_THROTTLE_2,
    DD_AEROENGINE_THROTTLE_3,
    DD_AEROENGINE_THROTTLE_4,
    DD_AEROENGINE_THROTTLE_5,

    DD_AEROENGINE_FAILED_0,
    DD_AEROENGINE_FAILED_1,
    DD_AEROENGINE_FAILED_2,
    DD_AEROENGINE_FAILED_3,
    DD_AEROENGINE_FAILED_4,
    DD_AEROENGINE_FAILED_5,

    DD_AEROENGINE_RPM_0,
    DD_AEROENGINE_RPM_1,
    DD_AEROENGINE_RPM_2,
    DD_AEROENGINE_RPM_3,
    DD_AEROENGINE_RPM_4,
    DD_AEROENGINE_RPM_5,

    DD_AIRSPEED, // speed in air

    DD_WING_AOA_0,
    DD_WING_AOA_1,
    DD_WING_AOA_2,
    DD_WING_AOA_3,
    DD_WING_AOA_4,
    DD_WING_AOA_5,

    DD_ALTITUDE,
    DD_ALTITUDE_STRING,

    DD_EDITOR_NODE_INFO, // editor node info string

    DD_ODOMETER_TOTAL,
    DD_ODOMETER_USER,

    DD_SIGNAL_TURNLEFT,  //!< Left blinker is lit.
    DD_SIGNAL_TURNRIGHT, //!< Right blinker is lit.
    DD_SIGNAL_WARNING,   //!< The warning-blink indicator is lit.

    DD_MAX
};

// this class is NOT intended to be thread safe - performance is required
class DashBoardManager : public ZeroedMemoryAllocator
{
public:
    DashBoardManager(void);
    ~DashBoardManager(void);

    // Getter / Setter
    bool _getBool(size_t key) { return data[key].data.value_bool; };
    int _getInt(size_t key) { return data[key].data.value_int; };
    float _getFloat(size_t key) { return data[key].data.value_float; };
    float getNumeric(size_t key);
    char* getChar(size_t key) { return data[key].data.value_char; };
    bool getEnabled(size_t key) { return data[key].enabled; };
    inline std::string getString(size_t key);

    void setBool(size_t key, bool val) { data[key].data.value_bool = val; };
    void setInt(size_t key, int val) { data[key].data.value_int = val; };
    void setFloat(size_t key, float val) { data[key].data.value_float = val; };
    void setChar(size_t key, const char* val) { strncpy(data[key].data.value_char, val, DD_MAXCHAR); };

    void setEnabled(size_t key, bool val) { data[key].enabled = val; };

    int getDataType(size_t key) { return data[key].type; };

    int getLinkIDForName(Ogre::String& str);

    int loadDashBoard(Ogre::String filename, bool textureLayer);

    void update(float& dt);
    void updateFeatures();

    bool WasDashboardLoaded() const { return (free_dashboard > 0); };

    void setVisible(bool visibility);
    void setVisible3d(bool visibility);
    bool getVisible() { return visible; };
    void windowResized();
protected:
    bool visible;
    dashData_t data[DD_MAX];
    DashBoard* dashboards[MAX_DASH];
    int free_dashboard;
};

class DashBoard : public ZeroedMemoryAllocator
{
    //friend class DashBoardManager;
public:
    DashBoard(DashBoardManager* manager, Ogre::String filename, bool textureLayer);
    ~DashBoard();

    void setVisible(bool visible);
    bool getVisible() { return m_overlay->isVisible(); };

    bool getIsTextureLayer() { return textureLayer; }

    void update(float& dt);
    void updateFeatures();

    void windowResized();

protected:
    Ogre::Overlay* m_overlay = nullptr;
    DashBoardManager* manager;
    Ogre::String filename;
    bool visible, textureLayer;
    std::string prefix;

    enum
    {
        ANIM_NONE,
        ANIM_ROTATE,
        ANIM_SCALE,
        ANIM_TEXTFORMAT,
        ANIM_TEXTSTRING,
        ANIM_LAMP,
        ANIM_SERIES,
        ANIM_TRANSLATE,
        ANIM_TEXTCOLOR
    };

    enum
    {
        DIRECTION_NONE,
        DIRECTION_UP,
        DIRECTION_RIGHT,
        DIRECTION_DOWN,
        DIRECTION_LEFT
    };

    enum
    {
        CONDITION_NONE,
        CONDITION_GREATER,
        CONDITION_LESSER
    };

    // linking attributes
    typedef struct layoutLink_t
    {
        int linkID = 0; // DD_*
        char animationType = ANIM_NONE; // ANIM_*
        int condition = CONDITION_NONE; // CONDITION_*
        float conditionArgument = 0.f;
        char direction = DIRECTION_NONE; // DIRECTION_*

        Ogre::OverlayElement* element = nullptr;
        std::vector<Ogre::MaterialPtr> materials;

        float last = 9999.f; // force initial update
        bool lastState = true;

        // --- OBSOLETE (pre-OVERDASH) ---
        float wmin; // rotation/offset whatever (widget min/max)
        float wmax;
        float vmin; // value min/max
        float vmax;

        char format[255]; // string format
        char texture[255]; // texture filename
        char format_neg_zero[255]; //!< Test for undesired '-0.0' on display. Only for link type "format". Empty if not applicable.

        MyGUI::Widget* widget;
        MyGUI::RotatingSkin* rotImg;
        MyGUI::ImageBox* img;
        MyGUI::TextBox* txt;
        MyGUI::IntSize initialSize;
        MyGUI::IntPoint initialPosition;
        // END OBSOLETE
    } layoutLink_t;

    bool setupLampAnim(layoutLink_t& ctrl);
    bool setupSeriesAnim(layoutLink_t& ctrl);
    void setupElement(Ogre::OverlayElement* elem);
    void loadLayout(Ogre::String filename);

    std::vector<layoutLink_t> controls;
};

} // namespace RoR
