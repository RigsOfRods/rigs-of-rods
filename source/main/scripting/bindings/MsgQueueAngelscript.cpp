/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "ScriptEvents.h"
#include "AngelScriptBindings.h"
#include "Application.h"
#include <angelscript.h>

using namespace AngelScript;
using namespace RoR;

void RoR::RegisterMessageQueue(asIScriptEngine* engine)
{
    int result;

    // enum MsgType
    result = engine->RegisterEnum("MsgType"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("MsgType", "MSG_INVALID", MSG_INVALID); ROR_ASSERT(result >= 0);
    // Application
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_SHUTDOWN_REQUESTED", MSG_APP_SHUTDOWN_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_SCREENSHOT_REQUESTED", MSG_APP_SCREENSHOT_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_DISPLAY_FULLSCREEN_REQUESTED", MSG_APP_DISPLAY_FULLSCREEN_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_DISPLAY_WINDOWED_REQUESTED", MSG_APP_DISPLAY_WINDOWED_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_MODCACHE_LOAD_REQUESTED", MSG_APP_MODCACHE_LOAD_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_MODCACHE_UPDATE_REQUESTED", MSG_APP_MODCACHE_UPDATE_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_MODCACHE_PURGE_REQUESTED", MSG_APP_MODCACHE_PURGE_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_LOAD_SCRIPT_REQUESTED", MSG_APP_LOAD_SCRIPT_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_UNLOAD_SCRIPT_REQUESTED", MSG_APP_UNLOAD_SCRIPT_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_APP_SCRIPT_THREAD_STATUS", MSG_APP_SCRIPT_THREAD_STATUS); ROR_ASSERT(result >= 0);
    // Networking
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_CONNECT_REQUESTED", MSG_NET_CONNECT_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_CONNECT_STARTED", MSG_NET_CONNECT_STARTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_CONNECT_PROGRESS", MSG_NET_CONNECT_PROGRESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_CONNECT_SUCCESS", MSG_NET_CONNECT_SUCCESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_CONNECT_FAILURE", MSG_NET_CONNECT_FAILURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_SERVER_KICK", MSG_NET_SERVER_KICK); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_DISCONNECT_REQUESTED", MSG_NET_DISCONNECT_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_USER_DISCONNECT", MSG_NET_USER_DISCONNECT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_RECV_ERROR", MSG_NET_RECV_ERROR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_REFRESH_SERVERLIST_SUCCESS", MSG_NET_REFRESH_SERVERLIST_SUCCESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_REFRESH_SERVERLIST_FAILURE", MSG_NET_REFRESH_SERVERLIST_FAILURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_REFRESH_REPOLIST_SUCCESS", MSG_NET_REFRESH_REPOLIST_SUCCESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_OPEN_RESOURCE_SUCCESS", MSG_NET_OPEN_RESOURCE_SUCCESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_REFRESH_REPOLIST_FAILURE", MSG_NET_REFRESH_REPOLIST_FAILURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_NET_REFRESH_AI_PRESETS", MSG_NET_REFRESH_AI_PRESETS); ROR_ASSERT(result >= 0);
    // Simulation
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_PAUSE_REQUESTED", MSG_SIM_PAUSE_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_UNPAUSE_REQUESTED", MSG_SIM_UNPAUSE_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_LOAD_TERRN_REQUESTED", MSG_SIM_LOAD_TERRN_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_LOAD_SAVEGAME_REQUESTED", MSG_SIM_LOAD_SAVEGAME_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_UNLOAD_TERRN_REQUESTED", MSG_SIM_UNLOAD_TERRN_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_SPAWN_ACTOR_REQUESTED", MSG_SIM_SPAWN_ACTOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_MODIFY_ACTOR_REQUESTED", MSG_SIM_MODIFY_ACTOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_DELETE_ACTOR_REQUESTED", MSG_SIM_DELETE_ACTOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_SEAT_PLAYER_REQUESTED", MSG_SIM_SEAT_PLAYER_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_TELEPORT_PLAYER_REQUESTED", MSG_SIM_TELEPORT_PLAYER_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_HIDE_NET_ACTOR_REQUESTED", MSG_SIM_HIDE_NET_ACTOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED", MSG_SIM_UNHIDE_NET_ACTOR_REQUESTED); ROR_ASSERT(result >= 0);
    // GUI
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_OPEN_MENU_REQUESTED", MSG_GUI_OPEN_MENU_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_CLOSE_MENU_REQUESTED", MSG_GUI_CLOSE_MENU_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_OPEN_SELECTOR_REQUESTED", MSG_GUI_OPEN_SELECTOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_CLOSE_SELECTOR_REQUESTED", MSG_GUI_CLOSE_SELECTOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_MP_CLIENTS_REFRESH", MSG_GUI_MP_CLIENTS_REFRESH); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED", MSG_GUI_SHOW_MESSAGE_BOX_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_DOWNLOAD_PROGRESS", MSG_GUI_DOWNLOAD_PROGRESS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_GUI_DOWNLOAD_FINISHED", MSG_GUI_DOWNLOAD_FINISHED); ROR_ASSERT(result >= 0);
    // Editing
    result = engine->RegisterEnumValue("MsgType", "MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED", MSG_EDI_MODIFY_GROUNDMODEL_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED", MSG_EDI_ENTER_TERRN_EDITOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED", MSG_EDI_LEAVE_TERRN_EDITOR_REQUESTED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("MsgType", "MSG_EDI_RELOAD_BUNDLE_REQUESTED", MSG_EDI_RELOAD_BUNDLE_REQUESTED); ROR_ASSERT(result >= 0);

}
