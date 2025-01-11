![Rigs of Rods](doc/images/RoR_Logo_full.png?raw=true)

[![Join Discord](https://img.shields.io/discord/136544456244461568.svg?style=flat-square)](https://discord.gg/rigsofrods) ![downloads](https://flat.badgen.net/github/assets-dl/RigsOfRods/rigs-of-rods)
![release](https://flat.badgen.net/github/release/RigsOfRods/rigs-of-rods)
![contributors](https://flat.badgen.net/github/contributors/RigsOfRods/rigs-of-rods)
![issues-pr](https://flat.badgen.net/github/open-prs/RigsOfRods/rigs-of-rods)
![last commit](https://flat.badgen.net/github/last-commit/RigsOfRods/rigs-of-rods)
[![Translations](https://hosted.weblate.org/widgets/rigs-of-rods/-/game/svg-badge.svg)](https://hosted.weblate.org/projects/rigs-of-rods/)
[![Coverity Scan Status](https://img.shields.io/coverity/scan/16646.svg?style=flat-square)](https://scan.coverity.com/projects/rigsofrods-rigs-of-rods)
[![Build game](https://github.com/RigsOfRods/rigs-of-rods/workflows/Build%20game/badge.svg)](https://github.com/RigsOfRods/rigs-of-rods/actions?query=workflow%3A%22Build+game%22)


_Rigs of Rods_ is a free/libre soft-body physics simulator mainly targeted at simulating vehicle physics in real-time. The soft-body physics system is based on [mass-spring-damper theory](https://en.wikipedia.org/wiki/Mass-spring-damper_model).

#### Trailer:

[![Rigs of Rods Trailer](http://img.youtube.com/vi/bRbQ4OaljWs/0.jpg)](http://www.youtube.com/watch?v=bRbQ4OaljWs)

## Supported platforms:
* Windows
* Linux

## Feature Overview
* real-time soft-body physics simulation, enabling:
  * realistic terrain-interaction of vehicles
  * realistic deformation and crashes
  * realistic interaction between vehicles and loads
  * realistic simulation of:
    * wheeled-vehicles, such as
      * cars
      * trucks
      * monster trucks
      * busses
      * [ground effect vehicles](https://en.wikipedia.org/wiki/Ground_effect_(cars))
    * tracked vehicles (experimental)
    * 2-wheeled vehicles (experimental)
    * boats
    * submarines (experimental)
    * trains
    * airplanes
    * helicopters
    * loads
    * prototypical, experimental, and fictional vehicles
* ability to enter and exit vehicles and walk around freely
* ability to spawn and use several vehicles of any type within the same session on the same map
* ingame content browser for community-created content and mods
* multiplayer support with client-server architecture and dedicated servers
  * ingame server-browser
  * dedicated servers can both be used to host online as well as LAN games
* ingame terrain-editor
* scripting support for modding via [AngelScript](https://angelcode.com/angelscript/)
* self-driving AI
  * easily create your own waypoints for AI to follow ingame
* continuous day and night cycle with realistic positioning and movement of celestial objects depending on time of day
* dynamic lighting with shadows
* vehicle modding (exchange vehicle parts with ingame tools)
* racing on race tracks with online leaderboards


### Gameplay
Rigs of Rods is not intended for any particular use case. Due to its diverse feature set, the large amount of community-created content and the living community, Rigs of Rods should be seen as a sandbox offering a plethora of possibilities.

Some examples include:
* roleplaying
* (school) bus driving
* cargo lifting and transportation
* driving and operating heavy equipment and utility vehicles, such as
  * cranes
  * forklifts
  * boom lifts
  * scissor lifts
  * trailers
  * road trains
* flying
* train driving
* monster truck events
* crash simulations
* racing
* vehicle modding and tuning
* testing of experimental and fictional vehicles
* drifting
* rock trawling
* mudding
* experimenting, messing around, and having fun with physics

To get a better understanding of what you can do in Rigs of Rods, see

* [Things you can do in Rigs of Rods](doc/Things%20you%20can%20do%20in%20Rigs%20of%20Rods.pdf)
* [Rigs of Rods 2010 promo](https://www.youtube.com/watch?v=3A6OHnAD_Pc)
* [Rigs of Rods 2015 trailer](http://www.youtube.com/watch?v=bRbQ4OaljWs)
* [Rigs of Rods 2022.12 release trailer](https://www.youtube.com/watch?v=OagiMx2zwTA)

## Further references
* [Official Website](https://www.rigsofrods.org/)
* [Official documentation](http://docs.rigsofrods.org/)
* [Official forum](https://forum.rigsofrods.org/)
* [Discord](https://discord.gg/rigsofrods)
* [Content Repository](https://forum.rigsofrods.org/resources/)
* [Source code repository](https://github.com/RigsOfRods/rigs-of-rods)
* [Developer Wiki](https://github.com/RigsOfRods/rigs-of-rods/wiki)
* [More documentation](doc/)
* [Translation interface](https://hosted.weblate.org/projects/rigs-of-rods/)


## Paths
This section describes where the directories that are most important to end-users are located on each supported operating system. Inside this Readme, these directories are referenced by name prefixed with `$`.

### Legend
```
$bin  - location of the compiled binaries
$res  - location of the resources/assets for the game
$user - location of user-created mods, configuration files, logs, screenshots
```

### Windows
```
$bin   = source\bin
$res   = source\bin\resources
$user  = Documents\My Games\Rigs of Rods
```

### Linux
```
$bin    = source/bin
$res    = source/bin/resources
$user   = ${HOME}/.rigsofrods
```

## Controls
Available commands depend on the vehicle you are in. For a graphical overview refer to [doc/keysheet.pdf](doc/keysheet.pdf)  
For an indepth view refer to [this Documentation page](https://docs.rigsofrods.org/gameplay/controls-config/) or ``` $user/config/input.map ```  

Please note that certain vehicles come with their own specific commands not represented in the above sources. In this case see the vehicle's documentation or go to the `Ingame Top Bar Menu` -> `Simulation` -> `Show vehicle description`.  
Rigs of Rods can also be played with Gamepads, Joysticks, Wheels and other controllers. Rigs of Rods supports Force Feedback.

### Basic controls:

| key                            | effect                                               |
|--------------------------------|------------------------------------------------------|
| arrow keys                     | move, steer, accelerate, decelarate                  |
| mouse+rightclick               | rotate camera                                        |
| mousewheel+rightclick          | camera zoom                                          |
| C                              | change camera perspective                            |
| T                              | display vehicle stats                                |
| CTRL+T                         | show vehicle description								|
| BACKSPACE                      | reset vehicle at current location                    |
| I                              | reset vehicle to spawnpoint                          |
| TAB                            | show minimap                                         |
| ESC                            | show menu                                            |
| A/Z                            | trucks: shift up/down                                |
| CTRL+HOME                      | aircrafts: start engine                              |
| Page Up/Down                   | aircrafts: increase/decrease throttle                |
| F1/F2                          | helicopters: lift up/down                            |
| Print Screen                   | create screenshot in `$user/screenshots` folder      |


## Content/Mods
Rigs of Rods only comes with a very small selection of vehicles and terrains. For the best experience download some mods from the [Rigs of Rods Mod Repository](https://forum.rigsofrods.org/resources/). For convenience, use the ingame content browser for downloading. It can be found in the main menu when clicking on the `Repository` button. The [Showrooms subforum](https://forum.rigsofrods.org/#showrooms.11) may contain additional content not found in the Mod Repository.  
If you want to get going quickly, have a look at the content packs. These can be found on the webpage of the [Mod Repository](https://forum.rigsofrods.org/resources/categories/content-packs.10/).


## Configuration files
- Default location:  
    ``` $user/config/* ```
- They are created by Rigs of Rods on first use


## Command-line options

* `-map <mapname>`
    * loads map `<mapname>` on startup. Example:
        * ``` RoR.exe -map aspen ```
    * note: do not add `.terrn2` file format extension
* `-truck <truckfile>`
    * loads a truck on startup. Example:
        * ``` RoR.exe -map oahu -truck semi.truck ```
        * ``` RoR.exe -map oahu -truck an-12.airplane ```
* `-enter`
    * enter truck provided with `-truck` option on startup
* `-setup`
    * displays OGRE3D settings dialog instead of loading settings from `ogre.cfg`
* `-help`
    * displays help for command line arguments


## Compiling
For instructions refer to [BUILDING.md](BUILDING.md). Also see the [Developer Wiki](https://github.com/RigsOfRods/rigs-of-rods/wiki).


## License of Rigs of Rods

Copyright (c) 2005-2013 Pierre-Michel Ricordel  
Copyright (c) 2007-2013 Thomas Fischer  
Copyright (c) 2009-2020 Rigs of Rods Contributors

Rigs of Rods went open source under GPLv2 or later on the 8th of February, 2009.

Rigs of Rods is now licensed under GPLv3 or later:
```
Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
```

* For the full license text see [COPYING](COPYING)
* For involved authors see [AUTHORS.md](AUTHORS.md)
* For licenses of used libraries see [DEPENDENCIES.md](DEPENDENCIES.md)
