Truck file format (technical spec)
==================================

Truck is a human friendly text format created and used by Rigs of Rods (www.rigsofrods.org).
Find user documentation at http://docs.rigsofrods.org/vehicle-creation/fileformat-truck.

This spec applies to Rigs of Rods version 0.4.5 and further.
This spec covers all prior versions of the format: 1, 2, 3, 450 (discontinued)
See also 'fileformatversion'.

Syntax
------

The format is line-based and processed linearly from top to down. Order is key.

Lines starting with semicolon ";" or slash "/" (documented as '//') are comments.
Placing ';' or '/' anywhere else will not be considered a comment, but parsed as regular data!

Lines are treated as values separated by separators. 
Possible separators: space, tabulator, comma ",", colon ":" or pipe "|".
Multiple separators in a row (even if different from each other) are treated as one.

The first line of the file (non-comment & non-empty) is a title. 

Any following line may either begin with a keyword, or (most commonly) define
gameplay elements depending on previous keywords. Many also affect all following
lines. Keywords are case-insensitive, the lettercasing in the list below
is given historically (it's how the contributor coded it).

Segments
--------

The file can be split to parts called "segments", which are grouped to "configurations".

Configurations are created using 'sectionconfig'. When spawning the truck, the user\
must select exactly one configuration. When spawning the truck, segments belonging
to the configuration will be processed (in order), other segments will be skipped.

A segment begins with keyword "section", optionally followed by list of configurations.
If no configurations are given, the segment belongs to all configs.

An "implicit segment" is a segment outside "section/end_section" pair.
In code, it's indicated with `RoR::Truck::Segment::defined_explicitly`.
One is always created at the very start of the file, another is created
after each 'end_section'.

Segments can't overlap - you can't enter another 'segment' when already in 'segment'.

Sections
--------

The file is separated to sections. This is the type of line you'll see most of
the time in truck files. Examples: globals/nodes/beams.

Sections begin with a line containing nothing but a keyword.
Each following line defines one element.
Section ends when another section is started, or a special
closing keyword is given, or a segment is ended.   
    
Directives
----------

A directive is a line with keyword and arguments.
A directive can be used anywhere, there is no limit.
It never changes current section or segment, but usually
modifies default values for all lines which come after.
Some also change a previously/subsequently defined element.
See comments in the keyword list below.

List of keywords
----------------

advdrag                      SECTION      
add_animation                DIRECTIVE  Special syntax
airbrakes                    SECTION      
animators                    SECTION      Special syntax: values separated by comma, options separated by '|'.
AntiLockBrakes               DIRECTIVE       
author                       DIRECTIVE  
axles                        SECTION      Special syntax
beams                        SECTION      
brakes                       SECTION
cab                          SECTION      
camerarail                   SECTION      
cameras                      SECTION      
cinecam                      SECTION      
collisionboxes               SECTION      
commands                     SECTION      
commands2                    SECTION      
comment                      SECTION - All lines except 'end_comment' will be skipped as commentary.    
contacters                   SECTION      
cruisecontrol                SECTION      
description                  SECTION - All lines except 'end_comment' will be loaded as description.
detacher_group               DIRECTIVE  
disabledefaultsounds         DIRECTIVE  
end                          DIRECTIVE - Ends the truck loading. All following lines are ignored.
end_comment                  END OF SECTION - Warning: Following lines belong to section before 'comment'.
end_description              END OF SECTION - Warning: Following lines belong to no section!
end_section                  END OF SEGMENT - Warning: Following lines belong to section before 'section'.
enable_advanced_deformation  DIRECTIVE  
engine                       SECTION      
engoption                    SECTION      
engturbo                     SECTION      
envmap                       SECTION      
exhausts                     SECTION      
extcamera                    DIRECTIVE     
forwardcommands              DIRECTIVE  
fileformatversion            DIRECTIVE     
fileinfo                     DIRECTIVE     
fixes                        SECTION      
flares                       SECTION      
flares2                      SECTION      
flexbodies                   SECTION      
flexbody_camera_mode         DIRECTIVE  
flexbodywheels               SECTION      
forset                       DIRECTIVE    Required 1x after each flexbody.  
fusedrag                     SECTION      
globals                      SECTION      
guid                         DIRECTIVE  
guisettings                  SECTION      
help                         SECTION      
hideInChooser                DIRECTIVE  
hookgroup                    SECTION      
hooks                        SECTION      
hydros                       SECTION      
importcommands               DIRECTIVE  
interaxles                   SECTION      Special syntax
lockgroups                   SECTION      
lockgroup_default_nolock     DIRECTIVE  
managedmaterials             SECTION      
materialflarebindings        SECTION      
meshwheels                   SECTION      
meshwheels2                  SECTION      
minimass                     SECTION      
nodecollision                SECTION      
nodes                        SECTION      
nodes2                       SECTION      
particles                    SECTION      
pistonprops                  SECTION      
prop_camera_mode             DIRECTIVE  
props                        SECTION      
railgroups                   SECTION      
rescuer                      DIRECTIVE  
rigidifiers                  SECTION      
rollon                       DIRECTIVE  
ropables                     SECTION      
ropes                        SECTION      
rotators                     SECTION      
rotators2                    SECTION      
screwprops                   SECTION      
sectionconfig                DIRECTIVE   Special - defines conf. names for 'section'
section                      SEGMENT     
set_beam_defaults            DIRECTIVE  
set_beam_defaults_scale      DIRECTIVE  
set_collision_range          DIRECTIVE  
set_default_minimass         DIRECTIVE
set_inertia_defaults         DIRECTIVE  
set_managedmaterials_options DIRECTIVE  
set_node_defaults            DIRECTIVE  
set_shadows                  DIRECTIVE  
set_skeleton_settings        DIRECTIVE  
shocks                       SECTION      
shocks2                      SECTION      
shocks3                      SECTION      
slidenode_connect_instantly  DIRECTIVE  
slidenodes                   SECTION      
SlopeBrake                   SECTION      
soundsources                 SECTION      
soundsources2                SECTION      
soundsources3                SECTION      
speedlimiter                 DIRECTIVE        
submesh                      DIRECTIVE - begin a new submesh.      
submesh_groundmodel          DIRECTIVE
texcoords                    SECTION        
ties                         SECTION      
torquecurve                  SECTION      
TractionControl              DIRECTIVE      
transfercase                 SECTION            
triggers                     SECTION      
turbojets                    SECTION      
turboprops                   SECTION      
turboprops2                  SECTION      
videocamera                  SECTION      
wheels                       SECTION      
wheels2                      SECTION      
wings                        SECTION    
