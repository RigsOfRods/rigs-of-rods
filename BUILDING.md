# Building instructions
Please refer to https://github.com/RigsOfRods/rigs-of-rods/wiki

# Dependencies
### Core requirements:
* C/C++ compiler with support for C++11 (e.g. gcc >= 4.8)
* cmake: >= 3.16

### Required dependencies

* OGRE:  1.11.6.1
* OpenAL: >= 1.18
* OIS:  1.4
* MyGUI:  3.4.0
* fmt: >= 6
* RapidJSON: >= 1.1

### Optional dependencies

* SocketW: >= 3.10
* AngelScript:  2.35.1
* CURL: >= 7.6
* Caelum:  0.6.3
* PagedGeometry:  1.2.1
* discord_rpc:  3.4.0
* OpenSSL: >= 1.1.1

**Notes:**
* AngelScript
  * Required for scripting (AI, racing, server mods...)
  * When building without AS this has to be removed in resources/particles/water.particle:  
  `affector FireExtinguisher {	effectiveness 	1 }`
* Caelum
  * Sky plugin: provides dynamic sky with time of day, weather and clouds
* SocketW
  * Required for network play
* nvidia-cg-toolkit
  * Required for Cg shader effects which some mods use
  * Not libre software
* PagedGeometry
  * Required to display vegetation
* discord_rpc
  * Used to display the "Now Playing" section in a Discord user's profile


## CMake options

### Core options

`ROR_DEPENDENCY_DIR`
This is used to set the path to the folder built by https://github.com/RigsOfRods/ror-dependencies
