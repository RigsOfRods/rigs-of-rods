![Rigs of Rods](doc/images/RoR_Logo_full.png?raw=true)

[![Join Discord](https://img.shields.io/discord/136544456244461568.svg?style=flat-square)](https://discord.gg/rigsofrods) ![downloads](https://flat.badgen.net/github/assets-dl/RigsOfRods/rigs-of-rods)
![release](https://flat.badgen.net/github/release/RigsOfRods/rigs-of-rods)
![contributors](https://flat.badgen.net/github/contributors/RigsOfRods/rigs-of-rods)
![issues-pr](https://flat.badgen.net/github/open-prs/RigsOfRods/rigs-of-rods)
![last commit](https://flat.badgen.net/github/last-commit/RigsOfRods/rigs-of-rods)



|  Build status 	|  [![Coverity Scan Status](https://img.shields.io/coverity/scan/16646.svg?style=flat-square)](https://scan.coverity.com/projects/rigsofrods-rigs-of-rods)	|
|---------------	|------------------	|
| Linux:        	| [![travis build Status](https://flat.badgen.net/travis/RigsOfRods/rigs-of-rods/master?icon=travis)](https://travis-ci.org/RigsOfRods/rigs-of-rods)                                    	|
| Windows:      	| [![appveyor build Status](https://flat.badgen.net/appveyor/ci/AnotherFoxGuy/rigs-of-rods/master?icon=appveyor)](https://ci.appveyor.com/project/AnotherFoxGuy/rigs-of-rods) 	|


Rigs of Rods is a free/libre soft-body physics simulator mainly targeted at simulating vehicle physics. The soft-body physics system is based on mass-spring-damper theory.  
For a simple overview of the features Rigs of Rods provides please refer to [doc/Things you can do in Rigs of Rods.pdf](doc/Things%20you%20can%20do%20in%20Rigs%20of%20Rods.pdf)  


#### Trailer:

[![Rigs of Rods Trailer](http://img.youtube.com/vi/bRbQ4OaljWs/0.jpg)](http://www.youtube.com/watch?v=bRbQ4OaljWs)

## Supported platforms:
* Windows
* Linux
* OS X

## Further documentation
* Website: https://rigsofrods.org/
* Documentation: http://docs.rigsofrods.org/
* Developer Wiki: https://github.com/RigsOfRods/rigs-of-rods/wiki
* Forum: https://forum.rigsofrods.org/
* Mod Repository: http://repository.rigsofrods.org/
* Github: https://github.com/RigsOfRods/rigs-of-rods
* Discord: https://discord.gg/rigsofrods
* [doc/](doc/)
* Translation interface: https://www.transifex.com/rigsofrods/rigs-of-rods/


## Paths
$bin  - compiled binaries  
$res  - resources/assets for the game  
$user - user-created mods, configuration files, logs, screenshots  

Windows:  
$bin   = source\bin  
$res   = source\bin\resources  
$user  = Documents\Rigs of Rods   

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
For an indepth view refer to ``` $user/config/input.map ```  
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
| Print Screen                   | create screenshot in $user/screenshots folder        |


## Content/Mods
Rigs of Rods only comes with a very small selection of vehicles and terrains. For the best experience download some mods from the [Rigs of Rods Mod Repository](http://repository.rigsofrods.org/). The [Showroom Subforum](https://forum.rigsofrods.org/showrooms/) may contain additional content not found in the Mod Repository.  
If you want to get going quickly have a look at modpacks which can be found in the Mod Repository as well.


## Configuration files
- Default location:  
    ``` $user/config/* ```
- They are created by RoRConfig on first use


## Command-line options

* -map \<mapname\>
    * loads map \<mapname\> on startup. Example:
        * ``` RoR.exe -map aspen ```
    * note: do not add .terrn2 file format extension
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
