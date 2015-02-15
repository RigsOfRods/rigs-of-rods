# Rigs of Rods

Copyright (c) 2005-2013 Pierre-Michel Ricordel  
Copyright (c) 2007-2013 Thomas Fischer  
Copyright (c) 2013-2015 Petr Ohlidal  

This is Rigs of Rods, an open source vehicle simulator focussed on simulating vehicle physics.
For a simple overview of the features Rigs of Rods provides please refer to doc/Things you can do in Rigs of Rods.pdf

#### Trailer:

[![Rigs of Rods Trailer](http://img.youtube.com/vi/3A6OHnAD_Pc/0.jpg)](http://www.youtube.com/watch?v=3A6OHnAD_Pc)

## Supported platforms:
* Windows
* Linux
* OS X


## Further documentation
* Website: http://www.rigsofrods.com/
* Wiki: http://www.rigsofrods.com/wiki/
* Developer wiki: http://www.rigsofrods.com/wiki/pages/Developer_Wiki_Portal
* Forum: http://www.rigsofrods.com/forum/
* Github: https://github.com/RigsOfRods/rigs-of-rods
* doc/ directory of source distribution


## Paths
$bin- Compiled binaries  
$res- Resources for the game  
$user- User-created modifiable data  

Windows:  
$bin   = source\bin  
$res   = source\bin\resources  
$user  = MyDocuments\Rigs of Rods  

Linux:  
$bin    = source/bin  
$res    = source/bin/resources  
$user   = ~/.rigsofrods  

OS X:  
$bin    = ?  
$res    = ?  
$user  = ?  


## Configuration files
- Default location:  
    $user/config/*
- They are created by RoRConfig on first use


## Command-line options

* -map \<mapname\>
    * loads the map on startup. Example:
        * ``` RoR.exe -map aspen ```
    * note: do not add the .terrn extension
* -truck \<truckfile\>
    * loads a truck on startup. For example: 
        * ``` RoR.exe -map oahu -truck semi.truck ```
        * ``` RoR.exe -map oahu -truck an-12.airplane ```
* -enter
    * Enter selected truck by -truck on startup
* -setup 
    * this shows the Ogre Settings Dialog instead of loading the settings from ogre.cfg 
* -help
    * shows a command line argument help

	
## Compiling
For instructions refer to BUILDING.md
	

## License of Rigs of Rods

Rigs of Rods is licensed under GPLv3 or later:
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

For licenses of used libraries see DEPENDENCIES.md
