This directory contains dashboard overlay elements.

Ogre Overlay is a simple and complete component for displaying 2D
(possibly also 3D) elements on screen. It uses standard Ogre
materials and supports text with TrueType fonts.
Most importantly, overlays can be created by non-programmers 
using text-based ".overlay" scripts, see Ogre manual:
https://ogrecave.github.io/ogre/api/latest/_overlay-_scripts.html

Overlay system is designed to be extensible via custom
OverlayElement and OverlayElementFactory subclasses.
Rigs of Rods uses this flexibility to create animated HUDs
(dashboards) which visualize gameplay data such as vehicle speed
or engine RPM. This is archieved by extra parameters
in .overlay scripts.

HOW TO USE:
The overlay elements and scripts are a replacement of older system
based on MyGUI's widgets XML .layout files. See documentation at
https://docs.rigsofrods.org/vehicle-creation/making-custom-hud/.
The overlay elements and parameters are a very close match,
see below notes.

AVAILABLE ELEMENTS:
* DashTextArea ~ Extends built-in 'TextArea'.

AVAILABLE 'link' VALUES:
Everything documented in the above manual.

AVAILABLE 'anim' VALUES:
* 'textstring' - Displays the value as-is. Only for 'DashTextArea'.
* 'lamp' - Switches materials "*-on" and "*-off", only for 'DashLamp'.

EXAMPLE:

Overlay script:
```
    overlay TestDash
    {
        zorder 100
        overlay_element TestDash/Root Panel
        {
            metrics_mode pixels
            width 200
            height 50
            material tracks/transred

            overlay_element TestDash/SpeedLabel TextArea
            {
                metrics_mode pixels
                left 10
                top 0
                font_name CyberbitEnglish
                char_height 20
                caption "Speed:"
            }

            overlay_element TestDash/SpeedDisplay DashTextArea
            {
                metrics_mode pixels
                left 10
                top 25
                font_name CyberbitEnglish
                char_height 20
                
                // ----- dashboard params -----
                link speedo_kph
                anim textstring
            }
        }
    }
```

Truck file:
```
    guisettings
    dashboard TestDash
```

