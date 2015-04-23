# Building instructions
Please refer to http://www.rigsofrods.com/wiki/pages/Compiling_Sources

## CMake options
##### Rigs of Rods core:  
| option                         | effect                                               |
|--------------------------------|------------------------------------------------------|
| CMAKE_BUILD_TYPE:STRING        | Build Type (DEBUG, RELEASE, RelWithDebInfo)          |
| ROR_BUILD_CONFIGURATOR:BOOL    | Build RoRConfig                                      |
| ROR_BUILD_TWODREPLAY:BOOL      | Build 2D Replay tool                                 |
| ROR_USE_ANGELSCRIPT:BOOL       | Build with Angelscript support                       |
| ROR_USE_CAELUM:BOOL            | Build with OGRE:Caelum sky plugin                    |
| ROR_USE_CURL:BOOL              | Build with curl for online services                  |
| ROR_USE_SOCKETW:BOOL           | Build with SocketW for cross-platform socket support |
| ROR_USE_MYGUI:BOOL             | Build with MyGUI GUI                                 |
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

For additional information refer to CMakeCache.txt after CMake has been configured at least once.
