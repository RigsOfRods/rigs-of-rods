Truck file format (technical spec)
==================================

Truck is a human friendly text format created and used by Rigs of Rods (www.rigsofrods.org).
Find user documentation at http://docs.rigsofrods.org/vehicle-creation/fileformat-truck.

This spec applies to Rigs of Rods version 0.4.5 and further.
This spec covers all prior versions of the format: 1, 2, 3, 450 (discontinued)
See also 'fileformatversion'.

Syntax
------

The format is line-based. Empty lines and comments are skipped.

Lines starting with semicolon ";" or slash "/" (documented as '//') are comments.
Placing ';' or '/' anywhere else will not be considered a comment, but parsed as regular data!

Each line is treated as values separated by separators. 
Possible separators: space, tabulator, comma ",", colon ":" or pipe "|".
Multiple separators in a row (even if different from each other) are treated as one.

Element types
-------------

Title
    The first non-comment & non-empty of the file is a title. 

Inline-section
    Example: author.
    Has a keyword at start of line and data follow on the same line.
    Following lines do not belong to any section.

Block-section
    Example: nodes.
    Begins with a line containing nothing but a keyword.
    All subsequent lines belong to it until another
      section (any type) is encountered or module closed.
    Some may have special syntax, i.e. 'animators'              

Multiline-description
    Single instance: "description/end_description".
    Begins with a line containing nothing but "description".
    Ends with a line containing nothing but "end_description".
    All lines in between belong to it. Keywords inside it are ignored.
    <!> Subsequent lines belong to no section.
    
Multiline-comment
    Single instance: "comment/end_comment".
    Begins with a line containing nothing but "description".
    Ends with a line containing nothing but "end_description".
    All lines in between belong to it. Keywords inside it are ignored.
    <!> Can appear inside any block-section, does not affect it.

Directive
    Example: set_beam_defaults
    Consists of keyword at the beginning of the line and data on the same line.
    May perform various task, usualy set global attributes or change
      behavior of the parsing.
    Directive may appear in any block-section.

Modularity
----------

The elements can be grouped into modules. Each module must belong to one or more configurations.

Directives 'sectionconfig' specify truck configurations the user can choose from.
Exactly one must be selected. If none, the first defined is used.
Syntax: "sectionconfig <number - unused> <name>"

A module begins with keyword "section". Syntax:
  "section <n> <config> [<config> ...]"
  where <n> is unused number and config is a name specified in sectionconfig.
Module ends with keyword "end_section".

All keywords are modular except:
    forwardcommands
    importcommands
    rollon
    rescuer
    disabledefaultsounds
    slidenode_connect_instantly
    guid
    hideInChooser

List of elements
----------------

Order is alphabetical, lettercase matches original docs (parsing is insensitive).

    NAME                         TYPE       NOTES
                                            
    advdrag                      BLOCK      
    add_animation                DIRECTIVE  Special syntax
    airbrakes                    BLOCK      
    animators                    BLOCK      Special syntax: values separated by comma, options separated by '|'.
    AntiLockBrakes               DIRECTIVE       
    author                       DIRECTIVE  
    axles                        BLOCK      Special syntax
    beams                        BLOCK      
    brakes                       BLOCK      
    camerarail                   BLOCK      
    cameras                      BLOCK      
    cinecam                      BLOCK      
    collisionboxes               BLOCK      
    commands                     BLOCK      
    commands2                    BLOCK      
    comment                      COMMENT    
    contacters                   BLOCK      
    cruisecontrol                BLOCK      
    description                  DESCRIPTION      
    detacher_group               DIRECTIVE  
    disabledefaultsounds         DIRECTIVE  
    enable_advanced_deformation  DIRECTIVE  
    engine                       BLOCK      
    engoption                    BLOCK      
    engturbo                     BLOCK      
    envmap                       BLOCK      
    exhausts                     BLOCK      
    extcamera                    INLINE     
    forwardcommands              DIRECTIVE  
    fileformatversion            INLINE     
    fileinfo                     DIRECTIVE     
    fixes                        BLOCK      
    flares                       BLOCK      
    flares2                      BLOCK      
    flexbodies                   BLOCK      
    flexbody_camera_mode         DIRECTIVE  
    flexbodywheels               BLOCK      
    forset                       INLINE     
    fusedrag                     BLOCK      
    globals                      BLOCK      
    guid                         DIRECTIVE  
    guisettings                  BLOCK      
    help                         BLOCK      
    hideInChooser                DIRECTIVE  
    hookgroup                    BLOCK      
    hooks                        BLOCK      
    hydros                       BLOCK      
    importcommands               DIRECTIVE  
    interaxles                   BLOCK      Special syntax
    lockgroups                   BLOCK      
    lockgroup_default_nolock     DIRECTIVE  
    managedmaterials             BLOCK      
    materialflarebindings        BLOCK      
    meshwheels                   BLOCK      
    meshwheels2                  BLOCK      
    minimass                     BLOCK      
    nodecollision                BLOCK      
    nodes                        BLOCK      
    nodes2                       BLOCK      
    particles                    BLOCK      
    pistonprops                  BLOCK      
    prop_camera_mode             DIRECTIVE  
    props                        BLOCK      
    railgroups                   BLOCK      
    rescuer                      DIRECTIVE  
    rigidifiers                  BLOCK      
    rollon                       DIRECTIVE  
    ropables                     BLOCK      
    ropes                        BLOCK      
    rotators                     BLOCK      
    rotators2                    BLOCK      
    screwprops                   BLOCK      
    sectionconfig                DIRECTIVE     
    section                      MODULE     
    set_beam_defaults            DIRECTIVE  
    set_beam_defaults_scale      DIRECTIVE  
    set_collision_range          DIRECTIVE  
    set_default_minimass         DIRECTIVE
    set_inertia_defaults         DIRECTIVE  
    set_managedmaterials_options DIRECTIVE  
    set_node_defaults            DIRECTIVE  
    set_shadows                  DIRECTIVE  
    set_skeleton_settings        DIRECTIVE  
    shocks                       BLOCK      
    shocks2                      BLOCK      
    shocks3                      BLOCK      
    slidenode_connect_instantly  DIRECTIVE  
    slidenodes                   BLOCK      
    SlopeBrake                   BLOCK      
    soundsources                 BLOCK      
    soundsources2                BLOCK      
    soundsources3                BLOCK      
    speedlimiter                 DIRECTIVE        
    submesh                      BLOCK      
    submesh_groundmodel          DIRECTIVE        
    ties                         BLOCK      
    torquecurve                  BLOCK      
    TractionControl              DIRECTIVE      
    transfercase                 BLOCK            
    triggers                     BLOCK      
    turbojets                    BLOCK      
    turboprops                   BLOCK      
    turboprops2                  BLOCK      
    videocamera                  BLOCK      
    wheels                       BLOCK      
    wheels2                      BLOCK      
    wings                        BLOCK      

Compatibility
-------------

Existing content places a lot of compatibility constraints on the format:
* Modules (section/end_section) must support:
  - nodes2 with the same names in all modules (KickerRampV2.load)
  - managedmaterials, props, help, soundsources, flexbodies, videocamera
  - globals, author (many mods in archive - because of skins)
  - flares (archive: BBV.zip - BBV95.truck)
  - meshwheels, axles, commands2 (archive: Mercedess Actros 8x8_ampliroll.truck)
  - engine, engoption, torquecurve (archive: ple002_SuperBus.truck, RockHopper.truck)
  - //multiple sectionconfig names//(archive: ackermann_moving_trailer.zip)
  - contacters (Betzi-tourliner.zip)
  - TractionControl (archive: golf cart)
  - brakes, nodes (archive: Blue&White-ScaniaR470TL.truck)
  

Since RoR v0.4.5, all elements fully support named nodes.
In previous RoR versions, support was partial and undocumented.

Since RoR v0.4.5 the order of elements in truckfile doesn't matter.
The spawn process follows a pre-defined order of elements. 
Exceptions:
    add_animation (must be after section 'prop')
    flexbody_camera_mode (must be after section 'flexbody')
In previous versions, the order of elements defined the final result
and some combinations resulted in a crash.

Since v0.4.5, keyword 'sectionconfig' is no longer necessary and parser silently ignores it.
Modules are created as keyword 'section' is encountered.    

Since v0.4.5 sections 'nodes' and 'nodes2' both support named nodes.

Since v0.4.5 sections 'commands' & 'commands2' are unified.
The only difference is in compress & expand ratios.
Both support the same flags and can use multiple flags at once.

Since v0.4.5 subsection 'flexbodies'/'forset' supports named nodes.
However, ranges are only supported for nubered nodes.

Since v0.4.5 the 'end' keyword is no longer needed. Parser silently ignores it.
In previous versions, it was critical - without it, RoR crashed.

Since v0.4.5 the optional param 'wings/control_type' now has default 'n'
Old parser had no default, though the param is optional.

DOCUMENTATION
=============




SPECIAL SYNTAXES
================

      

SECTION `FORSET`
----------------

Keyword: `forset`

Parsing as pseudocode:
~~~~~~~~~~~~~~~~~~~~~
    line.trim()
    IF (line == "forset"):  // Keyword "forset" alone
        abort("Invalid line.")
    line = line.substring(6) // Strip "forset"
    FOR EACH tok IN line.split(","): // Including empty strings
        IF tok.contains("-"):
            values = tok.split("-")
            node_a = 0;
            IF (values[0] == ""):
                node_a = values[0].to_uint();
            ADD_RANGE(node_a, values[1].to_uint())
        ELSE:
            ADD_NODE(tok.to_uint())
~~~~~~~~~~~~~~~~~~~~~

All whitespace on the line is ignored. 
Int parsing: `std::strtoul()`, bad trailing chars are silently ignored.
Syntax: 
    1.  "forset" string (6 characters), no space at the end.
    2.  Items delimited by ",". On each side of each "," there is
        max 1 item. Empty/invalid string parses as node num `0`.
        Examples: "forset," -> 2 items `0`, "forset,," -> 3 items '0'
Acceptable item forms:
    *  Single node number (int >= 0): 123
    *  Node range (int >= 0): "123-456"
    *  Single node ID (string) 
Line containing only "forset" is skipped with warning.
When multiple nodes are separated just by space/tab, the 1st node
    is parsed, the others ignored: `forset1 2 3, 4` -> "1, 4"
When a node range has more than 2 nodes, they're ignored.
    `forset1-2-3,4-5-6` -> [1-2],[4-5]

Hyphen rules:
    * Item "---" (at least 1 hyphen) parse as [0]
    * Item "-1" parses as [0-1], "--1" and longer parse as `-1` and return
        value around  4294967286 (unsigned int overflow).
    * Item "1-" (any number of "-") parse as [1-0]
    Valid examples:
        
        `forset1-5, 10 - 20 ,roof123, 1 2,3 4, 1-2-3,4-5-6,-----,`
            -> [1-5],[10-20],[roof123],[1],[3],[1-2],[4-5],[0],[0]
        
        `forset 1-2-3 , 4-5-6,   -7,   8-,  -9-,          --10,  11--, 100`
            -> [1-2]  ,[4-5] ,[0-7],[8-0],[0-9],[0-4294967286],[11-0],[100]

        `forset6--,-66--,--66--,---6--,--7-, ---,7-`
            -> [6-0], [0-66], [0-4294967230],[0-0],[0-4294967289],[0-0],[7-0]
