These files are adopted from ror-server.
See https://github.com/RigsOfRods/ror-server/tree/4a7109ae2d9a081ccfdad8cc696dc54efe49acb3

All filenames had "ServerScript*" prepended to them (unless it was there already).
Changes from the original are marked with "//RIGSOFRODS"

Created by ohlidalp, 2026 (see https://github.com/RigsOfRods/rigs-of-rods/pull/3393)

PURPOSE
=======

To make gameplay scripts (such as race system) usable both in singleplayer (with AI bots)
and multiplayer (with actual players under server authority).

USAGE
=====

Place server scripts to directory indicated by 'sys_server_scripts_dir'
That's 'Documents\My Games\Rigs of Rods\server_scripts' under MSWindows.
Tip: use the example script provided with rorserver (also well documented):
https://github.com/RigsOfRods/ror-server/blob/master/contrib/example-script.as

While in menu or singleplayer, open ingame console and say 'loadserverscript <name>'.
In case of any error, inspect "RoRServerScript.log" in logs directory.
The game will show classic Client List UI on top right, with you as server admin.
To stop the script, say 'unloadserverscript'.

In simulation, the top menubar 'Actors' menu will also behave as in multiplayer,
showing vehicles sorted by player/bot who spawned them.

To add bots, use the classic 'Vehicle AI' menu in top menubar.
These will appear as ranked players in the Client List UI.
To remove all bots, just press [Stop all] in the 'Vehicle AI' menu.
To remove individual bots, use standard `!kick <uid>` command,
or alternatively kill the script from Console UI/Script Monitor menu.

SERVER COMMANDS
===============

!help ~ prints built-in commands.
!version ~ prints "Local script"
!list ~ lists connected users.
!kick <uid> <message> ~ kicks a bot. Unlike in rorserver, kick message isn't supported.
!say <uid> <message> ~ currently dummy because bots cannot receive chat messages or game commands yet.

No other commmads are built-in, but may be provided by the script.

LOCAL PEEROPTIONS
=================

The 'mute actors' peeropt behaves identically as with rorserver.
No other peeropts are supported.

SCRIPT CALLBACKS
================

All existing callbacks are supported:
    void main() ~ required to exist as global function, invoked on startup.
    void playerAdded(int uid) ~ executed when player or bot joins. Player auto-joins when the server script is started.
    void playerDeleted(int uid, int crashed) ~ executed when bot leaves by any means. NOTE: RoRserver doesn't seem to invoke 'playerDeleted' on clean disconnect by player, but we do it for consistency
    int streamAdded(int uid, StreamRegister@ reg) ~ executed when player or bot spawns an actor. Returns `broadcastType` which determines how the message is treated.
    int playerChat(int uid, const string &in msg) ~ ONLY ONE AT A TIME ~ executed when player sends a chat message. Returns `broadcastType` which determines how the message is treated.
    void gameCmd(int uid, const string &in cmd) ~ ONLY ONE AT A TIME ~ invoked when a script running on client calls `game.sendGameCmd()`
    void frameStep(float dt_millis) ~ executed periodically, the parameter is delta time (time since last execution) in milliseconds.
    void curlStatus(curlStatusType type, int n1, int n2, string displayname, string message) ~ Provides progress and result info, see `server.curlRequestAsync()`; for CURL_STATUS_PROGRESS, n1 = bytes downloaded, n2 = total bytes; otherwise n1 = CURL return code, n2 = HTTP result code.

SCRIPT FUNCTIONS
================

Most functions work just like they do in rorserver.
    
The following are stubs, doing nothing and returning 0 or "":
    game.getServerIpAddress()
    game.getUserIpAddress(int uid)
    game.get_version()
    game.get_maxClients()
    game.get_serverName()
    game.get_IPAddr()
    game.get_listenPort()
    game.get_serverMode()
    game.get_owner()
    game.get_website()
    game.get_ircServ()
    game.get_voipServ()
    game.broadcastUserInfo(int uid)
    
