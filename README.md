# Rigs of Rods

[![Join the chat at gitter.im](https://img.shields.io/gitter/room/RigsOfRods/rigs-of-rods.svg?style=flat-square)](https://gitter.im/RigsOfRods/rigs-of-rods) ![downloads](https://img.shields.io/github/downloads/RigsOfRods/rigs-of-rods/latest/total.svg?style=flat-square)
![release](https://img.shields.io/github/release/RigsOfRods/rigs-of-rods.svg?style=flat-square)
![contributors](https://img.shields.io/github/contributors/RigsOfRods/rigs-of-rods.svg?style=flat-square)
![issues-pr](https://img.shields.io/github/issues-pr/RigsOfRods/rigs-of-rods.svg?style=flat-square)



|  Build status 	|                                                                                                                                                                             	|
|---------------	|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------	|
| Linux:        	| [![travis build Status](https://img.shields.io/travis/RigsOfRods/rigs-of-rods.svg?style=flat-square)](https://travis-ci.org/RigsOfRods/rigs-of-rods)                                    	|
| Windows:      	| [![appveyor build Status](https://img.shields.io/appveyor/ci/AnotherFoxGuy/rigs-of-rods/master.svg?style=flat-square)](https://ci.appveyor.com/project/AnotherFoxGuy/rigs-of-rods) 	|


Rigs of Rods is a free/libre soft-body physics simulator mainly targeted at simulating vehicle physics. The soft-body physics system is based on mass-spring-damper theory.  
For a simple overview of the features Rigs of Rods provides please refer to [doc/Things you can do in Rigs of Rods.pdf](doc/Things you can do in Rigs of Rods.pdf)  


#### Trailer:

[![Rigs of Rods Trailer](http://img.youtube.com/vi/bRbQ4OaljWs/0.jpg)](http://www.youtube.com/watch?v=bRbQ4OaljWs)

## Supported platforms:
* Windows
* Linux
* OS X

## Further documentation
* Website: https://rigsofrods.org/
* Wiki: http://docs.rigsofrods.org/
* Developer Wiki: http://developer.rigsofrods.org/
* Forum: https://forum.rigsofrods.org/
* Mod Repository: https://repository.rigsofrods.org/
* Github: https://github.com/RigsOfRods/rigs-of-rods
* IRC: #rigsofrods and #rigsofrods-dev on irc.freenode.net
* Discord: http://discord.gg/0lHqnBd15Cfz5g94
* [doc/](doc/)
* Translation interface: https://www.transifex.com/rigsofrods/rigs-of-rods/


## Paths
$bin  - compiled binaries  
$res  - resources/assets for the game  
$user - user-created mods, configuration files, logs, screenshots  

Windows:  
$bin   = source\bin  
$res   = source\bin\resources  
$user  = MyDocuments\Rigs of Rods  

Linux:  
$bin    = source/bin  
$res    = source/bin/resources  
$user   = ~/.rigsofrods  

OS X:  
$bin    = source/bin  
$res    = source/bin/resources  
$user   = ~/.rigsofrods  


## Controls
Available commands depend on the vehicle you are in. For a graphical overview refer to [doc/keysheet.pdf](doc/keysheet.pdf)  
For an indepth view refer to ``` $user/.rigsofrods/config/input.map ```  
Please note that certain vehicles come with their own specific commands not represented in the above sources. In this case see the vehicle's documentation or go to Menu -> Simulation -> Show vehicle description.  
Rigs is Rods can also be played with Gamepads, Joysticks, Wheels and other controllers, including support for Force Feedback.

##### Basic controls:  

| key                            | effect                                               |
|--------------------------------|------------------------------------------------------|
| arrow keys                     | move, steer, accelerate, decelarate                  |
| mouse+rightclick               | rotate camera                                        |
| mousewheel+rightclick          | camera zoom                                          |
| C                              | change camera perspective                            |
| T                              | display vehicle stats                                |
| BACKSPACE                      | reset vehicle at current location                    |
| I                              | reset vehicle to spawnpoint                          |
| TAB                            | show minimap                                         |
| ESC                            | show menu                                            |
| A/Z                            | trucks: shift up/down                                |
| CTRL+HOME                      | aircrafts: start engine                              |
| Page Up/Down                   | aircrafts: increase/decrease throttle                |
| F1/F2                          | helicopters: lift up/down                            |
| Print Screen                   | create screenshot in $user/ folder                   |


## Content/Mods
Rigs of Rods only comes with a very small selection of vehicles and terrains. For the best experience download some mods from the [Rigs of Rods Mod Repository](http://www.repository.rigsofrods.org/). The [Showroom Subforum](http://forum.rigsofrods.org/forum-6.html) may contain additional content not found in the Mod Repository.  
If you want to get going quickly have a look at modpacks which can be found in the Mod Repository as well.


## Configuration files
- Default location:  
    ``` $user/config/* ```
- They are created by RoRConfig on first use


## Command-line options

* -map \<mapname\>
    * loads map \<mapname\> on startup. Example:
        * ``` RoR.exe -map aspen ```
    * note: do not add .terrn file format extension
* -truck \<truckfile\>
    * loads a truck on startup. Example:
        * ``` RoR.exe -map oahu -truck semi.truck ```
        * ``` RoR.exe -map oahu -truck an-12.airplane ```
* -enter
    * enter selected truck by -truck option on startup
* -setup
    * displays OGRE3D settings dialog instead of loading settings from ogre.cfg
* -help
    * displays help for command line arguments


## Compiling
For instructions refer to [BUILDING.md](BUILDING.md)


## License of Rigs of Rods

Copyright (c) 2005-2013 Pierre-Michel Ricordel  
Copyright (c) 2007-2013 Thomas Fischer  
Copyright (c) 2009-2015 Rigs of Rods Contributors

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
