# Building instructions
Please refer to https://github.com/RigsOfRods/rigs-of-rods/wiki

# Dependencies
### Core requirements:
* C/C++ compiler with support for C++11 (e.g. gcc >= 4.8)
* cmake: >= 3.16

### Required dependencies

* OGRE:  1.11.6.1
* OpenAL: >= 1.18
* OIS: >= 1.4
* MyGUI:  3.4.0
* fmt: >= 6
* RapidJSON: >= 1.1

### Optional dependencies

* SocketW: >= 3.10
* AngelScript:  2.32.0
* CURL: >= 7.6
* Caelum:  0.6.4
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

The Rigs of Rods build system allows to have on-demand fallback on Conan.
This means that if your (Linux) distribution doesn't ship a compatible version of a dependency you can use conan to install the missing dependency without having to install everything with conan.

`ROR_FORCE_SYSTEM_DEPENDENCIES`
Values: `[ON, OFF]`

* If this has been set to ON cmake will throw an error instead of falling pack to conan.

`ROR_LIB_PREFERENCE`
Values: `[SYSTEM, CONAN]`

* This will set the preferred method of obtaining dependencies.
See the explanation below for meaning the values.

`ROR_DEPENDENCY_DIR`

This is used to set the path to the folder built by https://github.com/RigsOfRods/ror-dependencies

#### **Explanation of the dependencies options values**

* `SYSTEM` This means that cmake will first search for the dependency on the system, if it can't find it will add it to the list of conan packages to install.
* `CONAN` This means that cmake will add the dependency to the list of conan packages to install.
* `OFF` This means that the dependency will not be searched for nor be installed with conan.

### Dependencies options

| Name | Values |
|------|--------|
| `ROR_USE_OGRE` | `SYSTEM`, `CONAN` |
| `ROR_USE_OPENAL` | `SYSTEM`, `CONAN` |
| `ROR_USE_OIS` | `SYSTEM`, `CONAN` |
| `ROR_USE_MYGUI` | `SYSTEM`, `CONAN` |
| `ROR_USE_SOCKETW` | `SYSTEM`, `CONAN`, `OFF` |
| `ROR_USE_ANGELSCRIPT` | `SYSTEM`, `CONAN`, `OFF` |
| `ROR_USE_CURL` | `SYSTEM`, `CONAN`, `OFF` |
| `ROR_USE_CAELUM` | `SYSTEM`, `CONAN`, `OFF` |
| `ROR_USE_PAGEDGEOMETRY` | `SYSTEM`, `CONAN`, `OFF` |
| `ROR_USE_FMT` | `SYSTEM`, `CONAN` |
| `ROR_USE_DISCORD_RPC` | `SYSTEM`, `CONAN`, `OFF` |
| `ROR_USE_RAPIDJSON` | `SYSTEM`, `CONAN` |
| `ROR_USE_OPENSSL` | `SYSTEM`, `CONAN`, `OFF` |