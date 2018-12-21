# Building instructions
Please refer to https://github.com/RigsOfRods/rigs-of-rods/wiki

# Dependencies
core requirements:
* C/C++ compiler with support for C++11 (e.g. gcc >= 4.8)
* boost: >= 1.50
* cmake: >= 2.8
* curl
* libssl
* libgtk 2.0
* mygui: >= 3.2.2
* ogre: 1.8.x or 1.9.x
* OIS: 1.3
* openal-soft (any version should work, crashes with 1.15.x)
* RapidJSON >= 1.1.0

optional but recommended:
* angelscript: 2.31.2
  * required for scripting (AI, racing, server mods...)
  * when building without AS this has to be removed in resources/particles/water.particle: "affector FireExtinguisher {	effectiveness 	1 }"
* caelum: >= 0.6.2, compatible with the OGRE version you chose
  * sky plugin: provides dynamic sky with time of day, weather and clouds
* mysocketw: latest from git
  * required for network play
* nvidia-cg-toolkit
  * required for Cg shader effects which some mods use
  * Not libre software
  * Requires Ogre to be compiled by hand
* paged geometry: latest from git
  * required to display vegetation

## CMake options
##### Rigs of Rods core:  
| option                         | effect                                               |
|--------------------------------|------------------------------------------------------|
| CMAKE_BUILD_TYPE:STRING        | Build Type (DEBUG, RELEASE, RelWithDebInfo)          |
| ROR_BUILD_CONFIGURATOR:BOOL    | Build RoRConfig                                      |
| ROR_USE_ANGELSCRIPT:BOOL       | Build with Angelscript support                       |
| ROR_USE_CAELUM:BOOL            | Build with OGRE:Caelum sky plugin                    |
| ROR_USE_CURL:BOOL              | Build with curl for online services                  |
| ROR_USE_SOCKETW:BOOL           | Build with SocketW for cross-platform socket support |
| ROR_USE_OPENAL:BOOL            | Build with OpenAL for sound                          |
| ROR_USE_PAGED:BOOL             | Build with OGRE:Paged Geometry for vegetation        |

##### Library specific:  
| option                         | effect                                               |
|--------------------------------|------------------------------------------------------|
| CMAKE_CXX_COMPILER:FILEPATH    | Path to C++ compiler                                 |
| ANGELSCRIPT_INCLUDE_DIRS:PATH  | Path to AngelScript header files                     |
| ANGELSCRIPT_LIBRARIES:FILEPATH | Path to AngelScript library                          |
| Boost_INCLUDE_DIR:PATH         | Path to Boost header files                           |
| Boost_LIBRARY_DIR:PATH         | Path to Boost library                                |
| CAELUM_INCLUDE_DIRS:PATH       | Path to Caelum header files                          |
| CAELUM_LIBRARIES:FILEPATH      | Path to Caelum library                               |
| CURL_INCLUDE_DIR:PATH          | Path to curl header files                            |
| CURL_LIBRARY:FILEPATH          | Path to curl library                                 |
| MYGUI_INCLUDE_DIRS:PATH        | Path to MyGUI header files                           |
| MYGUI_OGRE_PLATFORM:FILEPATH   | Path to MyGUI library                                |
| OPENAL_INCLUDE_DIR:PATH        | Path to OpenAL header files                          |
| OPENAL_LIBRARY:FILEPATH        | Path to OpenAL library                               |
| PAGED_INCLUDE_DIRS:PATH        | Path to Paged Geometry header files                  |
| PAGED_LIBRARIES:FILEPATH       | Path to Paged Geometry library                       |
| SOCKETW_INCLUDE_DIRS:PATH      | Path to SocketW header files                         |
| SOCKETW_LIBRARIES:FILEPATH     | Path to SocketW library                              |

For additional information refer to CMakeCache.txt after CMake has been configured at least once.
