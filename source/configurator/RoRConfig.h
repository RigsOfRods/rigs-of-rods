/*
    This source file is part of Rigs of Rods
    Copyright 2005-2009 Pierre-Michel Ricordel
    Copyright 2007-2009 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

#pragma once

/// @file   RoRConfig.h
/// @author Petr Ohlidal
/// @date   02/2017
/// @brief  Application-wide declarations for RoRConfig

#include <wx/defs.h>
#include <wx/string.h>

// Widget control-IDs
enum ControlIds
{
	ID_BUTTON_ADD_KEY = 1,
	ID_BUTTON_CANCEL,
	ID_BUTTON_CHECK_OPENCL,
	ID_BUTTON_CHECK_OPENCL_BW,
	ID_BUTTON_CLEAR_CACHE,
	ID_BUTTON_DELETE_KEY,
	ID_BUTTON_GET_USER_TOKEN,
	ID_BUTTON_LOAD_KEYMAP,
	ID_BUTTON_PLAY,
	ID_BUTTON_REGEN_CACHE,
	ID_BUTTON_RESTORE,
	ID_BUTTON_SAVE,
	ID_BUTTON_SAVE_KEYMAP,
	ID_BUTTON_UPDATE_ROR,
	ID_BUTTON_SCAN_CONTROLLERS,
	ID_CHANGED_NOTEBOOK_1,
	ID_CHANGED_NOTEBOOK_2,
	ID_CHANGED_NOTEBOOK_3,
	ID_CHOICE_LANGUAGE,
	ID_CHOICE_RENDERER,
	ID_CHOICE_SHADOW,
	ID_SCROLL_FORCE_FEEDBACK,
	ID_SCROLL_FPS_LIMITER,
	ID_SCROLL_SIGHT_RANGE,
	ID_SCROLL_VOLUME,
	ID_TIMER_CONTROLS,
	ID_TIMER_UPDATE_RESET,
};

// ========== SystemTools.cpp ==========

wxString LoadInputDevicesInfo(WXWidget wx_window_handle);
