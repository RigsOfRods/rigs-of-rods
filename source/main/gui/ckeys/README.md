## Rigs of Rods

Adopted this utility for in-game keyboard help.

Derived from commit 98d897734c588c9b85bd36e7286405983ac1cbbc. Source file layout kept as-is.

## Crystal Keys (cKeys)

Program showing pressed keys and keyboard layouts.  
Currently works on Windows, for GNU/Linux it should be easy to port.  
Made as a support and visualization program for my keyboards CK3 and CK4 (more about them on [my website](https://cryham.tuxfamily.org/archives/portfolio/crystal-keyboard-3-and-4)).  
More about the program below and [here](https://cryham.tuxfamily.org/archives/portfolio/crystal-keys).  

## General

Used SFML for drawing all keys. Also for writting text in pressed list and (mouse over) key info.  
Keyboard is real-time scalable with slider. Fit button resizes window to match it. Reset restores default scale 1.00.  
For GUI elements used ImGui through ImGui-SFML (both are included in sources).  
Windows Keyboard Hook is used to get key states (Virtual Key codes).  

## Tutorials

**Earlier versions** in [releases](https://github.com/cryham/ckeys/releases): 0.1, 0.3 and 0.5  
were just demo programs for using [SFML](https://github.com/SFML/SFML), [ImGui](https://github.com/ocornut/imgui) and [TinyXML-2](https://github.com/leethomason/tinyxml2).  
And can serve as **tutorials or starting projects** for similar applications, for both GNU/Linux and Windows.  


## Layouts

Program reads custom keyboard layouts, from **JSON** files (using jsmn parser library).  
Which can be created, edited and saved from this web based editor: www.keyboard-layout-editor.com  
The default.json layout is the ANSI 104 preset.  

All `*.json` files located in `data` directory will be available in Layout combobox.  
Program supports only basic rectangular layouts, no rotation or styling.  
Additionally, I added own commands to json, which replace key names `"n:Name"` and assign scan code `"s:12"`.  

## Layers

Since version 1.0 it also reads **KLL** files ([specification](https://input.club/kll/), [repo](https://github.com/kiibohd/kll)).  
Basic parsing is done to get layer key bindings (mapping).  

For e.g. `ck3` keyboard the base map is `defaultMapCK3.kll`.  
All `ck3layer*.kll` files in `data` dir are read for layers. Maximum number is 8.  


## Compiling

Using [CMake](https://cmake.org/), it should be possible to build easily.  
I'm using Qt Creator IDE, which supports it.  

On GNU/Linux you'll need to get SFML installed. At least version 2.2.  
E.g. for Ubuntu (and Debian-based):  
`sudo apt-get install libsfml-dev`  

On Windows SFML needs the environment variable SFML_ROOT to be defined.  
After this the program should build.  

In sources there already are: [TinyXML2](https://github.com/leethomason/tinyxml2), [jsmn](https://github.com/zserge/jsmn), ImGui with ImGui-SFML and [Native File Dialog](https://github.com/mlabbe/nativefiledialog).  


## Screenshot with default layout

![](https://raw.githubusercontent.com/cryham/ckeys/master/screenshot.png)

## Screenshot with layers

![](https://raw.githubusercontent.com/cryham/ckeys/master/screenshot2.png)
