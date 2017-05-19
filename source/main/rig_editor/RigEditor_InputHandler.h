/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   RigEditor_InputHandler.h
    @date   06/2014
    @author Petr Ohlidal
*/

#pragma once

#include "BitFlags.h"
#include "RigEditor_Types.h"

#include <OISKeyboard.h>
#include <OISMouse.h>
#include <bitset>

namespace RoR
{

namespace RigEditor
{

/** Editor-specific input handler
* NOTE: class InputEngine is not useful because it's too tightly-coupled with simulation logic.
*/
class InputHandler:
    public OIS::MouseListener,
    public OIS::KeyListener
{
public:

    static const int KEY_MAPPING_ARRAY_SIZE = 256; // Maximum number of keys for OIS.

    struct Event
    {
        static const Event INVALID;
        static const Event ESCAPE;
        static const Event CAMERA_VIEW_FRONT;
        static const Event CAMERA_VIEW_SIDE;
        static const Event CAMERA_VIEW_TOP;
        static const Event CAMERA_VIEW_TOGGLE_PERSPECTIVE;
        static const Event NODES_DESELECT_OR_SELECT_ALL;
        static const Event GUI_SHOW_DELETE_MENU;
        static const Event NODES_EXTRUDE_SELECTED;

        Event(unsigned int index, const char * name):
            index(index),
            name(name)
        {}

        const char * name;
        unsigned int index;
    };

    struct Mode
    {
        static const Mode INVALID;
        static const Mode CREATE_NEW_NODE;
        static const Mode GRAB_NODES;

        Mode(unsigned int index, const char * name, bool key_enters, bool key_exits):
            index(index),
            name(name),
            key_enters(key_enters),
            key_exits(key_exits)
        {}

        const char *  name;
        unsigned int  index;
        bool          key_enters;
        bool          key_exits;
    };

    struct MouseMotionEvent
    {
        int rel_x;
        int rel_y;
        int rel_wheel;
        int abs_x;
        int abs_y;
        int abs_wheel;

        MouseMotionEvent();
        
        bool HasMoved() const
        {
            return rel_x != 0 || rel_y != 0;
        }

        void ResetRelativeMove()
        {
            rel_x = 0;
            rel_y = 0;
        }

        bool HasScrolled() const
        {
            return rel_wheel != 0;
        }

        void ResetRelativeScroll()
        {
            rel_wheel = 0;
        }

        void AddRelativeMove(int x, int y, int wheel)
        {
            rel_x += x;
            rel_y += y;
            rel_wheel += wheel;
        }

        Vector2int GetAbsolutePosition() const
        {
            return Vector2int(abs_x, abs_y);
        }

        Vector2int GetPreviousAbsolutePosition() const;
    };

    struct MouseButtonEvent
    {
        static const int RIGHT_BUTTON_PRESSED = BITMASK(1);
        static const int RIGHT_BUTTON_RELEASED = BITMASK(2);
        static const int RIGHT_BUTTON_IS_DOWN = BITMASK(3);

        static const int LEFT_BUTTON_PRESSED = BITMASK(4);
        static const int LEFT_BUTTON_RELEASED = BITMASK(5);
        static const int LEFT_BUTTON_IS_DOWN = BITMASK(6);

        static const int MIDDLE_BUTTON_PRESSED = BITMASK(7);
        static const int MIDDLE_BUTTON_RELEASED = BITMASK(8);
        static const int MIDDLE_BUTTON_IS_DOWN = BITMASK(9);

        static const int BUTTON_PRESS_INPUT_RECEIVED = BITMASK(10);
        static const int BUTTON_RELEASE_INPUT_RECEIVED = BITMASK(11);
        static const int ALL_BUTTON_PRESSES_HANDLED_BY_GUI = BITMASK(12);
        static const int ALL_BUTTON_RELEASES_HANDLED_BY_GUI = BITMASK(13);

        MouseButtonEvent():
            flags(0)
        {}

        void ResetEvents()
        {
            BITMASK_SET_0(flags, 
                RIGHT_BUTTON_PRESSED | RIGHT_BUTTON_RELEASED
                | LEFT_BUTTON_PRESSED | LEFT_BUTTON_RELEASED
                | MIDDLE_BUTTON_PRESSED	| MIDDLE_BUTTON_RELEASED
                | BUTTON_PRESS_INPUT_RECEIVED | BUTTON_RELEASE_INPUT_RECEIVED
                | ALL_BUTTON_PRESSES_HANDLED_BY_GUI | ALL_BUTTON_RELEASES_HANDLED_BY_GUI
                );
        }

        inline void ButtonPressInputReceived(bool handled_by_gui)
        {
            if (handled_by_gui && BITMASK_IS_0(flags, BUTTON_PRESS_INPUT_RECEIVED)) // First input + handled by GUI
            {
                BITMASK_SET_1(flags, ALL_BUTTON_PRESSES_HANDLED_BY_GUI);
            }
            else if (!handled_by_gui && BITMASK_IS_1(flags, ALL_BUTTON_PRESSES_HANDLED_BY_GUI)) // Any input + not handled by GUI
            {
                BITMASK_SET_0(flags, ALL_BUTTON_PRESSES_HANDLED_BY_GUI);
            }
            // else ALL_BUTTON_PRESSES_HANDLED_BY_GUI stays FALSE
            BITMASK_SET_1(flags, BUTTON_PRESS_INPUT_RECEIVED);
        }

        inline void ButtonReleaseInputReceived(bool handled_by_gui)
        {
            if (handled_by_gui && BITMASK_IS_0(flags, BUTTON_RELEASE_INPUT_RECEIVED)) // First input + handled by GUI
            {
                BITMASK_SET_1(flags, ALL_BUTTON_RELEASES_HANDLED_BY_GUI);
            }
            else if (!handled_by_gui && BITMASK_IS_1(flags, ALL_BUTTON_RELEASES_HANDLED_BY_GUI)) // Any input + not handled by GUI
            {
                BITMASK_SET_0(flags, ALL_BUTTON_RELEASES_HANDLED_BY_GUI);
            }
            // else ALL_BUTTON_PRESSES_HANDLED_BY_GUI stays FALSE
            BITMASK_SET_1(flags, BUTTON_RELEASE_INPUT_RECEIVED);
        }

        void RightButtonDown()
        {
            BITMASK_SET_1(flags, RIGHT_BUTTON_PRESSED);
            BITMASK_SET_1(flags, RIGHT_BUTTON_IS_DOWN);
        }

        void RightButtonUp()
        {
            BITMASK_SET_1(flags, RIGHT_BUTTON_RELEASED);
            BITMASK_SET_0(flags, RIGHT_BUTTON_IS_DOWN);
        }

        void LeftButtonDown()
        {
            BITMASK_SET_1(flags, LEFT_BUTTON_PRESSED);
            BITMASK_SET_1(flags, LEFT_BUTTON_IS_DOWN);
        }

        void LeftButtonUp()
        {
            BITMASK_SET_1(flags, LEFT_BUTTON_RELEASED);
            BITMASK_SET_0(flags, LEFT_BUTTON_IS_DOWN);
        }

        void MiddleButtonDown()
        {
            BITMASK_SET_1(flags, MIDDLE_BUTTON_PRESSED);
            BITMASK_SET_1(flags, MIDDLE_BUTTON_IS_DOWN);
        }

        void MiddleButtonUp()
        {
            BITMASK_SET_1(flags, MIDDLE_BUTTON_RELEASED);
            BITMASK_SET_0(flags, MIDDLE_BUTTON_IS_DOWN);
        }

        bool IsRightButtonDown() const
        {
            return BITMASK_IS_1(flags, RIGHT_BUTTON_IS_DOWN);
        }

        bool IsLeftButtonDown() const
        {
            return BITMASK_IS_1(flags, LEFT_BUTTON_IS_DOWN);
        }

        bool IsMiddleButtonDown() const
        {
            return BITMASK_IS_1(flags, MIDDLE_BUTTON_IS_DOWN);
        }

        bool WasLeftButtonPressed() const
        {
            return BITMASK_IS_1(flags, LEFT_BUTTON_PRESSED);
        }

        bool WasRightButtonPressed() const
        {
            return BITMASK_IS_1(flags, RIGHT_BUTTON_PRESSED);
        }

        bool WasMiddleButtonPressed() const
        {
            return BITMASK_IS_1(flags, MIDDLE_BUTTON_PRESSED);
        }

    private:

        int flags;
    };

    InputHandler();

    ~InputHandler()
    {}

    bool WasEventFired(Event const & event);

    bool IsModeActive(Mode const & mode);

    bool WasModeEntered(Mode const & mode);
    
    bool WasModeExited(Mode const & mode);

    void EnterMode(Mode const & mode);

    void ExitMode(Mode const & mode);

    void ResetEvents();

    MouseMotionEvent const & GetMouseMotionEvent();

    MouseButtonEvent const & GetMouseButtonEvent();

    /* OIS::KeyListener */
    bool keyPressed( const OIS::KeyEvent &arg );
    bool keyReleased( const OIS::KeyEvent &arg );

    /* OIS MouseListener */
    bool mouseMoved( const OIS::MouseEvent &arg );
    bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

private:

    void SetupDefaultKeyMappings();

    const Event*       m_event_key_mappings[KEY_MAPPING_ARRAY_SIZE];
    std::bitset<64>    m_events_fired;
    const Mode*        m_mode_key_mappings[KEY_MAPPING_ARRAY_SIZE];
    std::bitset<32>    m_active_modes;
    std::bitset<32>    m_modes_entered;
    std::bitset<32>    m_modes_exited;

    MouseMotionEvent   m_mouse_motion_event;
    MouseButtonEvent   m_mouse_button_event;
};

} // namespace RigEditor

} // namespace RoR
