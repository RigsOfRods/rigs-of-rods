================================================================================
    RoR Rig Definition Parser
================================================================================

This is stand-aloner parser for rig-definition file format (called '.truck')
used in Rigs of Rods game.

Discussion on RoR forums:
http://www.rigsofrods.org/threads/108818

________________________________________________________________________________
TERMINOLOGY:
    Logical parts of rig-def file are refered to as follows:

        Section (block section)
            Example: nodes.
            Has a keyword at the beginning line and subsequent lines carry data.
        Inline-section
            Example: author.
            Has a keyword at start of line and data follow on the same line.
        Directive
            Example: set_beam_defaults
            Like inline-section, but it's data change results of subsequent or previous parsing.

        Module
            Keywords: 'section' and 'end_section'
            A chunk of file which represents an optional modification of the vehicle.

________________________________________________________________________________
COMPATIBILITY NOTES:
    All elements now fully support named nodes.

    All elements are order-independent, except:
        add_animation (must be after section 'prop')
        flexbody_camera_mode (must be after section 'flexbody')

    All elements can be modularized using 'section' keyword, except:
        author
        description
        disabledefaultsounds
        enable_advanced_deformation
        fileformatversion
        fileinfo
        forwardcommands
        guid
        hideInChooser
        importcommands
        lockgroup_default_nolock
        minimass
        rescuer
        rollon

    Keyword 'sectionconfig' is no longer necessary and parser silently ignores it.
        Modules are created as keyword 'section' is encountered.

    Keywords 'nodes' and 'nodes2' have been unified. Both support named nodes.

    Sections 'commands' & 'commands2' are unified; the only difference is in compress & expand ratios.
        Both support the same flags and can use multiple flags at once.

    Subsection 'flexbodies'/'forset' supports named nodes.
        However, ranges are only supported for nubered nodes.

    The 'end' keyword is no longer needed. Parser silently ignores it.

    The optional param 'wings/control_type' now has default 'n'
        Old parser had no default, though the param is optional.

________________________________________________________________________________
PROJECT STATUS:

	--------------------
	s = has struct
	_ = needs no struct
	r = has regex
	~ = needs no regex
	p = has parser logic
	t = tested
    C = correct(ed) regex (final check)

	X = not supported
	--------------------


X       SECTION        advdrag
srptC   DIRECTIVE      add_animation
srptC   SECTION        airbrakes
srptC   SECTION        animators
srptC   SECTION        AntiLockBrakes                (defaults in: SerializedRig::loadTruck)
srptC   INLINE-SECTION author
srptC   SECTION        axles
srptC   SECTION        beams
srptC   SECTION        brakes
srptC   SECTION        camerarail
srptC   SECTION        cameras
srptC   SECTION        cinecam
srptC   SECTION        collisionboxes
srptC   SECTION        commands
srptC   SECTION        commands2
_~ptC   BLOCK-COMMENT  comment
_~ptC   SECTION        contacters
srptC   SECTION        cruisecontrol
_rptC   SECTION        description
_rptC   DIRECTIVE      detacher_group
_rptC   DIRECTIVE      disabledefaultsounds
_rptC   DIRECTIVE      enable_advanced_deformation
srptC   SECTION        engine
srptC   SECTION        engoption
_~X     SECTION        envmap
srptC   SECTION        exhausts
srptC   INLINE-SECTION extcamera                    (Defaults: SerializedRig ctor)
_rptC   DIRECTIVE      forwardcommands
_rptC   INLINE-SECTION fileformatversion
srptC   INLINE-SECTION fileinfo                     (Defaults: SerializedRig ctor)
_~ptC   SECTION        fixes
srptC   SECTION        flares
srptC   SECTION        flares2
srptC   SECTION        flexbodies
srptC   DIRECTIVE      flexbody_camera_mode
rsptC   SECTION        flexbodywheels
srptC   SECTION        fusedrag
srptC   SECTION        globals
_rptC   INLINE-SECTION guid
srptC   SECTION        guisettings
srptC   SECTION        help
_rptC   DIRECTIVE      hideInChooser
X       SECTION        hookgroup
srptC   SECTION        hooks
srptC   SECTION        hydros
_rptC   DIRECTIVE      importcommands
srptC   SECTION        lockgroups
srptC   DIRECTIVE      lockgroup_default_nolock
srptC   SECTION        managedmaterials
srptC   SECTION        materialflarebindings
srptC   SECTION        meshwheels
srptC   SECTION        meshwheels2
_rptC   SECTION        minimass
srptC   SECTION        nodecollision
srptC   SECTION        nodes
srptC   SECTION        nodes2
srptC   SECTION        particles
srptC   SECTION        pistonprops
srptC   DIRECTIVE      prop_camera_mode
srptC   SECTION        props
srpC    SECTION        railgroups
_rptC   DIRECTIVE      rescuer
X       SECTION        rigidifiers
_rptC   DIRECTIVE      rollon
srptC   SECTION        ropables
srptC   SECTION        ropes
srptC   SECTION        rotators
srptC   SECTION        rotators2
srptC   SECTION        screwprops
_rptC   CONFIGURATOR   sectionconfig
srptC   CONFIGURATOR   section
srptC   DIRECTIVE      set_beam_defaults
srptC   DIRECTIVE      set_beam_defaults_scale
srptC   DIRECTIVE      set_inertia_defaults
srptC   DIRECTIVE      set_managedmaterials_options
srptC   DIRECTIVE      set_node_defaults
X       DIRECTIVE      set_shadows
srptC   DIRECTIVE      set_skeleton_settings
srptC   SECTION        shocks
srptC   SECTION        shocks2
_rptC   DIRECTIVE      slidenode_connect_instantly
srpt    SECTION        slidenodes
srptC   SECTION        SlopeBrake
srptC   SECTION        soundsources
srptC   SECTION        soundsources2
X       SECTION        soundsources3
srptC   SECTION        speedlimiter
srptC   SECTION        submesh
srptC   SECTION        submesh_groundmodel
srptC   SECTION        ties
stpt    SECTION        torquecurve
srptC   SECTION        TractionControl
srptC   SECTION        triggers
srptC   SECTION        turbojets
srptC   SECTION        turboprops
X       SECTION        turboprops2
srptC   SECTION        videocamera
srptC   SECTION        wheels
srptC   SECTION        wheels2
srptC   SECTION        wings
