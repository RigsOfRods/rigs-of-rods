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


#include "Application.h"
#include "RefCountingObject.h"
#include "RefCountingObjectPtr.h"

#include <MyGUI.h>

#include <string>
#include <vector>

namespace RoR {

#define DD_MAXCHAR 255
#define DD_MAX_SCREWPROP  6
#define DD_MAX_AEROENGINE 6
#define DD_MAX_WING       6
#define DD_MAX_CUSTOM_VALUES 512
#define DD_MAX_GEOMETRIC_ANIMATIONS 10

#define MAX_CONTROLS      1024

    struct dataContainer_t
    {
        bool value_bool = false;
        int value_int = 0;
        float value_float = 0.f;
        char value_char[DD_MAXCHAR] = {};
    };

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
            enabled = false;
        }

        dashData_t(char type, const char* name) : type(type), name(name)
        {
            enabled = true;
        }
    } dashData_t;

    // DashData enum definition
    // important: if you add things here, also change the initialization in the constructor
    enum DashData
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

        DD_ODOMETER_TOTAL,
        DD_ODOMETER_USER,

        // Lights (mirrors RoRnet::Lightmask)

        DD_CUSTOM_LIGHT1,  //!< custom light 1 on
        DD_CUSTOM_LIGHT2,  //!< custom light 2 on
        DD_CUSTOM_LIGHT3,  //!< custom light 3 on
        DD_CUSTOM_LIGHT4,  //!< custom light 4 on
        DD_CUSTOM_LIGHT5,  //!< custom light 5 on
        DD_CUSTOM_LIGHT6,  //!< custom light 6 on
        DD_CUSTOM_LIGHT7,  //!< custom light 7 on
        DD_CUSTOM_LIGHT8,  //!< custom light 8 on
        DD_CUSTOM_LIGHT9,  //!< custom light 9 on
        DD_CUSTOM_LIGHT10, //!< custom light 10 on

        DD_HEADLIGHTS,
        DD_HIGHBEAMS,
        DD_FOGLIGHTS,
        DD_SIDELIGHTS,
        DD_BRAKE_LIGHTS,
        DD_REVERSE_LIGHT,
        DD_BEACONS,
        DD_LIGHTS_LEGACY,    //!< Alias of 'sidelights'

        DD_SIGNAL_TURNLEFT,  //!< Left blinker is lit.
        DD_SIGNAL_TURNRIGHT, //!< Right blinker is lit.
        DD_SIGNAL_WARNING,   //!< The warning-blink indicator is lit.

        DD_CUSTOMVALUE_START,
        DD_CUSTOMVALUE_END = DD_CUSTOMVALUE_START + DD_MAX_CUSTOM_VALUES,

        DD_MAX
    };

    enum LoadDashBoardFlags
    {
        LOADDASHBOARD_SCREEN_HUD = BITMASK(1), //!< Will be drawn to screen. Unless STACKABLE, it prevents the default dashboard from loading.
        LOADDASHBOARD_RTT_TEXTURE = BITMASK(2), //!< Will be drawn to texture. Unless STACKABLE, it prevents the default dashboard from loading.
        LOADDASHBOARD_STACKABLE = BITMASK(3) //!< Allows loading multiple dashboards at once (by default there's only one for screen and one for RTT).
    };

    // this class is NOT intended to be thread safe - performance is required
    class DashBoardManager : public RefCountingObject<DashBoardManager>
    {
    public:
        DashBoardManager(ActorPtr actor);
        virtual ~DashBoardManager() override;

        int registerCustomValue(Ogre::String& name, int dataType);

        // Getter / Setter
        bool _getBool(size_t key) { return key < DD_MAX ? data[key].data.value_bool : false; };
        int _getInt(size_t key) { return data[key].data.value_int; };
        float _getFloat(size_t key) { return data[key].data.value_float; };
        float getNumeric(size_t key);
        char* getChar(size_t key) { return key < DD_MAX ? data[key].data.value_char : nullptr; };
        bool getEnabled(size_t key) { return key < DD_MAX ? data[key].enabled : false; };

        void setBool(size_t key, bool val) { if (key < DD_MAX) data[key].data.value_bool = val; };
        void setInt(size_t key, int val) { if (key < DD_MAX) data[key].data.value_int = val; };
        void setFloat(size_t key, float val) { if (key < DD_MAX) data[key].data.value_float = val; };
        void setChar(size_t key, const char* val) { if (key < DD_MAX) strncpy(data[key].data.value_char, val, DD_MAXCHAR); };

        void setEnabled(size_t key, bool val) { if (key < DD_MAX) data[key].enabled = val; };

        int getDataType(size_t key) { return key < DD_MAX ? data[key].type : DC_INVALID; };

        int getLinkIDForName(Ogre::String& str);
        std::string getLinkNameForID(DashData id);

        void loadDashBoard(const std::string& filename, BitMask_t flags);

        void update(float dt);
        void updateFeatures();

        bool wasDashboardHudLoaded() const { return m_hud_loaded; };
        bool wasDashboardRttLoaded() const { return m_rtt_loaded; };

        void setVisible(bool visibility);
        void setVisible3d(bool visibility);
        bool getVisible() { return visible; };
        void windowResized();
    protected:
        std::string determineLayoutFromDashboardMod(CacheEntryPtr& entry, std::string const& basename);
        std::string determineTruckLayoutFromDashboardMod(Ogre::FileInfoListPtr& filelist);
        bool visible = false;
        dashData_t data[DD_MAX];
        std::vector<DashBoard*> m_dashboards;
        bool m_hud_loaded = false;
        bool m_rtt_loaded = false;
        ActorPtr m_actor;
        int registeredCustomValues = 0;
    };

    class DashBoard
    {
    public:
        DashBoard(DashBoardManager* manager, Ogre::String filename, bool textureLayer);
        ~DashBoard();

        void setVisible(bool visible, bool smooth = true);
        bool getVisible() { return visible; };

        bool getIsTextureLayer() { return textureLayer; }

        void update(float dt);
        void updateFeatures();

        void windowResized();

        float getSmoothNumeric(int linkID, float& lastVal);

    protected:
        DashBoardManager* manager;
        Ogre::String filename;
        MyGUI::VectorWidgetPtr widgets;
        MyGUI::WindowPtr mainWidget;
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

        typedef struct layoutGeometricAnimation_t
        {
            int linkID; // DD_*

            char animationType; // ANIM_*

            float wmin; // rotation/offset whatever (widget min/max)
            float wmax;
            float vmin; // value min/max
            float vmax;
            char direction; // DIRECTION_*
            float lastVal;
        } layoutGeometricAnimation_t;

        typedef struct layoutGraphicalAnimation_t
        {
            int linkID; // DD_*

            char animationType; // ANIM_*

            int condition; // CONDITION_*
            float conditionArgument;
            char format[255]; // string format
            char texture[255]; // texture filename
            char format_neg_zero[255]; //!< Test for undesired '-0.0' on display. Only for link type "format". Empty if not applicable.
            float lastVal;
        } layoutGraphicalAnimation_t;

        // linking attributes
        typedef struct layoutLink_t
        {
            int linkIDForVisibility;

            char name[255]; // widget name

            layoutGraphicalAnimation_t graphicalAnimation;
            layoutGeometricAnimation_t geometricAnimations[DD_MAX_GEOMETRIC_ANIMATIONS];
            int geometricAnimationCount;

            MyGUI::Widget* widget;
            MyGUI::RotatingSkin* rotImg;
            MyGUI::ImageBox* img;
            MyGUI::TextBox* txt;
            MyGUI::IntSize initialSize;
            MyGUI::IntPoint initialPosition;

            bool lastState;
        } layoutLink_t;

        void loadLayoutInternal();
        void loadLayoutRecursive(MyGUI::WidgetPtr ptr);
        bool parseLink(std::string& linkArgs, int& linkID, char& condition, float& conditionArgument, const std::string& filename, const std::string& name);
        layoutLink_t controls[MAX_CONTROLS];
        int free_controls;
    };

} // namespace RoR
