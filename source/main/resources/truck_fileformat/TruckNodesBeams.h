/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author Petr Ohlidal
/// @brief Node/beam outputting sections in truck format.

#pragma once

#include "Application.h"
#include "TruckPresets.h"
#include "SimData.h"

#include <string>

namespace RoR {
namespace Truck {

/// Keywords 'nodes*'
struct Node: Element
{
    struct Ref
    {
        Ref() {}
        Ref(NodeIdx_t idx): number(idx) {}
        Ref(NodeIdx_t idx, std::string const &name): number(idx), name(name) {}

        std::string const & Str() const        { return name; }
        NodeIdx_t      Num() const        { return number; }

        bool Compare   (Ref const & rhs) const { return number == rhs.number; }
        bool operator==(Ref const & rhs) const { return Compare(rhs); }
        bool operator!=(Ref const & rhs) const { return ! Compare(rhs); }

        bool     IsValidAnyState() const       { return number != node_t::INVALID_IDX || name != ""; } // LEGACY, to be removed

        void Invalidate() { number = node_t::INVALID_IDX; name = ""; }
        std::string ToString() const { return TOSTRING(number)+"/"+name; }

        NodeIdx_t number = node_t::INVALID_IDX;
        std::string name = "";
    };

    struct Range
    {
        Range(Node::Ref const & start, Node::Ref const & end):
            start(start),
            end(end)
        {}

        Range(Node::Ref const & single):
            start(single),
            end(single)
        {}

        bool IsRange() const { return start != end; }

        void SetSingle(Node::Ref const & ref)
        {
            start = ref;
            end = ref;
        }

        Node::Ref start;
        Node::Ref end;
    };

    enum Option: char
    {
        OPTION_n_MOUSE_GRAB         = 'n',
        OPTION_m_NO_MOUSE_GRAB      = 'm',
        OPTION_f_NO_SPARKS          = 'f',
        OPTION_x_EXHAUST_POINT      = 'x',
        OPTION_y_EXHAUST_DIRECTION  = 'y',
        OPTION_c_NO_GROUND_CONTACT  = 'c',
        OPTION_h_HOOK_POINT         = 'h',
        OPTION_e_TERRAIN_EDIT_POINT = 'e',
        OPTION_b_EXTRA_BUOYANCY     = 'b',
        OPTION_p_NO_PARTICLES       = 'p',
        OPTION_L_LOG                = 'L',
        OPTION_l_LOAD_WEIGHT        = 'l',
    };

    NodeIdx_t number = node_t::INVALID_IDX;
    std::string name = "";
    Ogre::Vector3 position = Ogre::Vector3::ZERO;
    std::string options;
    float load_weight_override = 0.f;
};

/// Section 'animators'
struct AeroAnimator
{
    static const unsigned int OPTION_THROTTLE = BITMASK(1);
    static const unsigned int OPTION_RPM      = BITMASK(2);
    static const unsigned int OPTION_TORQUE   = BITMASK(3);
    static const unsigned int OPTION_PITCH    = BITMASK(4);
    static const unsigned int OPTION_STATUS   = BITMASK(5);

    unsigned int flags      = 0u;
    unsigned int engine_idx = 0u;
};

/// Section 'animators'
struct Animator
{
    static const unsigned int OPTION_VISIBLE           = BITMASK(1);
    static const unsigned int OPTION_INVISIBLE         = BITMASK(2);
    static const unsigned int OPTION_AIRSPEED          = BITMASK(3);
    static const unsigned int OPTION_VERTICAL_VELOCITY = BITMASK(4);
    static const unsigned int OPTION_ALTIMETER_100K    = BITMASK(5);
    static const unsigned int OPTION_ALTIMETER_10K     = BITMASK(6);
    static const unsigned int OPTION_ALTIMETER_1K      = BITMASK(7);
    static const unsigned int OPTION_ANGLE_OF_ATTACK   = BITMASK(8);
    static const unsigned int OPTION_FLAP              = BITMASK(9);
    static const unsigned int OPTION_AIR_BRAKE         = BITMASK(10);
    static const unsigned int OPTION_ROLL              = BITMASK(11);
    static const unsigned int OPTION_PITCH             = BITMASK(12);
    static const unsigned int OPTION_BRAKES            = BITMASK(13);
    static const unsigned int OPTION_ACCEL             = BITMASK(14);
    static const unsigned int OPTION_CLUTCH            = BITMASK(15);
    static const unsigned int OPTION_SPEEDO            = BITMASK(16);
    static const unsigned int OPTION_TACHO             = BITMASK(17);
    static const unsigned int OPTION_TURBO             = BITMASK(18);
    static const unsigned int OPTION_PARKING           = BITMASK(19);
    static const unsigned int OPTION_SHIFT_LEFT_RIGHT  = BITMASK(20);
    static const unsigned int OPTION_SHIFT_BACK_FORTH  = BITMASK(21);
    static const unsigned int OPTION_SEQUENTIAL_SHIFT  = BITMASK(22);
    static const unsigned int OPTION_GEAR_SELECT       = BITMASK(23);
    static const unsigned int OPTION_TORQUE            = BITMASK(24);
    static const unsigned int OPTION_DIFFLOCK          = BITMASK(25);
    static const unsigned int OPTION_BOAT_RUDDER       = BITMASK(26);
    static const unsigned int OPTION_BOAT_THROTTLE     = BITMASK(27);
    static const unsigned int OPTION_SHORT_LIMIT       = BITMASK(28);
    static const unsigned int OPTION_LONG_LIMIT        = BITMASK(29);

    Node::Ref nodes[2];
    float lenghtening_factor = 0.f;
    int flags = 0;
    float short_limit = 0.f;
    float long_limit = 0.f;
    AeroAnimator aero_animator;
};

/// Section 'beams'
struct Beam
{
    Node::Ref nodes[2];
    float extension_break_limit = 0.f;
    bool _has_extension_break_limit = false;
    bool invisible = false;
    SpecialBeam bounded = SpecialBeam::REGULAR_BEAM;
};

/// Sections 'commands/commands2'
struct Command2
{
    unsigned int _format_version = 1;
    Node::Ref nodes[2];
    float shorten_rate = 0.f;
    float lengthen_rate = 0.f;
    float max_contraction = 0.f;
    float max_extension = 0.f;
    unsigned int contract_key = 0.f;
    unsigned int extend_key = 0.f;
    std::string description;
    OptionalInertia inertia;
    float affect_engine = false;
    bool needs_engine = true;
    bool plays_sound = true;

    bool option_i_invisible = false;
    bool option_r_rope = false;
    bool option_c_auto_center = false;
    bool option_f_not_faster = false;
    bool option_p_1press = false;
    bool option_o_1press_center = false;
};

/// Section 'hydros'
struct Hydro
{
    static const char OPTION_n_NORMAL                    = 'n';
    static const char OPTION_i_INVISIBLE                 = 'i';

    /* Useful for trucks */

    static const char OPTION_s_DISABLE_ON_HIGH_SPEED     = 's';

    /* Useful for planes: These can be used to control flight surfaces, or to create a thrust vectoring system.  */

    static const char OPTION_a_INPUT_AILERON             = 'a';
    static const char OPTION_r_INPUT_RUDDER              = 'r';
    static const char OPTION_e_INPUT_ELEVATOR            = 'e';
    static const char OPTION_u_INPUT_AILERON_ELEVATOR    = 'u';
    static const char OPTION_v_INPUT_InvAILERON_ELEVATOR = 'v';
    static const char OPTION_x_INPUT_AILERON_RUDDER      = 'x';
    static const char OPTION_y_INPUT_InvAILERON_RUDDER   = 'y';
    static const char OPTION_g_INPUT_ELEVATOR_RUDDER     = 'g';
    static const char OPTION_h_INPUT_InvELEVATOR_RUDDER  = 'h';

    inline bool HasFlag_a() { return options.find(Truck::Hydro::OPTION_a_INPUT_AILERON)             != std::string::npos; }
    inline bool HasFlag_e() { return options.find(Truck::Hydro::OPTION_e_INPUT_ELEVATOR)            != std::string::npos; }
    inline bool HasFlag_g() { return options.find(Truck::Hydro::OPTION_g_INPUT_ELEVATOR_RUDDER)     != std::string::npos; }
    inline bool HasFlag_h() { return options.find(Truck::Hydro::OPTION_h_INPUT_InvELEVATOR_RUDDER)  != std::string::npos; }
    inline bool HasFlag_i() { return options.find(Truck::Hydro::OPTION_i_INVISIBLE)                 != std::string::npos; }
    inline bool HasFlag_r() { return options.find(Truck::Hydro::OPTION_r_INPUT_RUDDER)              != std::string::npos; }
    inline bool HasFlag_s() { return options.find(Truck::Hydro::OPTION_s_DISABLE_ON_HIGH_SPEED)     != std::string::npos; }
    inline bool HasFlag_u() { return options.find(Truck::Hydro::OPTION_u_INPUT_AILERON_ELEVATOR)    != std::string::npos; }
    inline bool HasFlag_v() { return options.find(Truck::Hydro::OPTION_v_INPUT_InvAILERON_ELEVATOR) != std::string::npos; }
    inline bool HasFlag_x() { return options.find(Truck::Hydro::OPTION_x_INPUT_AILERON_RUDDER)      != std::string::npos; }
    inline bool HasFlag_y() { return options.find(Truck::Hydro::OPTION_y_INPUT_InvAILERON_RUDDER)   != std::string::npos; }

    inline void AddFlag(char flag) { options += flag; }

    Node::Ref nodes[2];
    float lenghtening_factor = 0.f;
    std::string options;
    OptionalInertia inertia;
};

/// Section 'ropes'
struct Rope
{
    Node::Ref root_node;
    Node::Ref end_node;
    bool invisible = false;
};

/// Section 'shocks'
struct Shock
{
    Shock();

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE    , HasOption_i_Invisible,   SetOption_i_Invisible) 
    // Stability active suspension can be made with "L" for suspension on the truck's left and "R" for suspension on the truck's right. 
    BITMASK_PROPERTY(options, 2, OPTION_L_ACTIVE_LEFT  , HasOption_L_ActiveLeft,  SetOption_L_ActiveLeft) 
    // Stability active suspension can be made with "L" for suspension on the truck's left and "R" for suspension on the truck's right. 
    BITMASK_PROPERTY(options, 3, OPTION_R_ACTIVE_RIGHT , HasOption_R_ActiveRight, SetOption_R_ActiveRight)
    BITMASK_PROPERTY(options, 4, OPTION_m_METRIC       , HasOption_m_Metric,      SetOption_m_Metric) 

    Node::Ref nodes[2];
    float spring_rate;         //!< The 'stiffness' of the shock. The higher the value, the less the shock will move for a given bump. 
    float damping;             //!< The 'resistance to motion' of the shock. The best value is given by this equation:  2 * sqrt(suspended mass * springness)
    float short_bound;         //!< Maximum contraction. The shortest length the shock can be, as a proportion of its original length. "0" means the shock will not be able to contract at all, "1" will let it contract all the way to zero length. If the shock tries to shorten more than this value allows, it will become as rigid as a normal beam. 
    float long_bound;          //!< Maximum extension. The longest length a shock can be, as a proportion of its original length. "0" means the shock will not be able to extend at all. "1" means the shock will be able to double its length. Higher values allow for longer extension.
    float precompression;      //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0. 
    unsigned int options;      //!< Bit flags.
};

/// Section 'shocks2'
struct Shock2
{
    Shock2();

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE       , HasOption_i_Invisible,      SetOption_i_Invisible) 
    // soft bump boundaries, use when shocks reach limiters too often and "jumprebound" (default is hard bump boundaries)
    BITMASK_PROPERTY(options, 2, OPTION_s_SOFT_BUMP_BOUNDS, HasOption_s_SoftBumpBounds, SetOption_s_SoftBumpBounds)
    // metric values for shortbound/longbound applying to the length of the beam.
    BITMASK_PROPERTY(options, 3, OPTION_m_METRIC          , HasOption_m_Metric,         SetOption_m_Metric)
    // Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)
    BITMASK_PROPERTY(options, 4, OPTION_M_ABSOLUTE_METRIC , HasOption_M_AbsoluteMetric, SetOption_M_AbsoluteMetric)  

    Node::Ref nodes[2];
    float spring_in;                  //!< Spring value applied when the shock is compressing.
    float damp_in;                    //!< Damping value applied when the shock is compressing. 
    float progress_factor_spring_in;  //!< Progression factor for springin. A value of 0 disables this option. 1...x as multipliers, example:maximum springrate == springrate + (factor*springrate)
    float progress_factor_damp_in;    //!< Progression factor for dampin. 0 = disabled, 1...x as multipliers, example:maximum dampingrate == springrate + (factor*dampingrate)
    float spring_out;                 //!< spring value applied when shock extending
    float damp_out;                   //!< damping value applied when shock extending
    float progress_factor_spring_out; //!< Progression factor springout, 0 = disabled, 1...x as multipliers, example:maximum springrate == springrate + (factor*springrate)
    float progress_factor_damp_out;   //!< Progression factor dampout, 0 = disabled, 1...x as multipliers, example:maximum dampingrate == springrate + (factor*dampingrate)
    float short_bound;                //!< Maximum contraction limit, in percentage ( 1.00 = 100% )
    float long_bound;                 //!< Maximum extension limit, in percentage ( 1.00 = 100% )
    float precompression;             //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0.  
    int options;             //!< Bit flags.
};

/// Section 'shocks3'
struct Shock3
{
    Shock3();

    BITMASK_PROPERTY(options, 1, OPTION_i_INVISIBLE       , HasOption_i_Invisible,      SetOption_i_Invisible) 
    // metric values for shortbound/longbound applying to the length of the beam.
    BITMASK_PROPERTY(options, 2, OPTION_m_METRIC          , HasOption_m_Metric,         SetOption_m_Metric)
    // Absolute metric values for shortbound/longbound, settings apply without regarding to the original length of the beam.(Use with caution, check ror.log for errors)
    BITMASK_PROPERTY(options, 3, OPTION_M_ABSOLUTE_METRIC , HasOption_M_AbsoluteMetric, SetOption_M_AbsoluteMetric)  

    Node::Ref nodes[2];
    float spring_in;                  //!< Spring value applied when the shock is compressing.
    float damp_in;                    //!< Damping value applied when the shock is compressing. 
    float spring_out;                 //!< Spring value applied when shock extending
    float damp_out;                   //!< Damping value applied when shock extending
    float damp_in_slow;               //!< Damping value applied when shock is commpressing slower than split in velocity
    float split_vel_in;               //!< Split velocity in (m/s) - threshold for slow / fast damping during compression
    float damp_in_fast;               //!< Damping value applied when shock is commpressing faster than split in velocity
    float damp_out_slow;              //!< Damping value applied when shock is commpressing slower than split out velocity
    float split_vel_out;              //!< Split velocity in (m/s) - threshold for slow / fast damping during extension
    float damp_out_fast;              //!< Damping value applied when shock is commpressing faster than split out velocity
    float short_bound;                //!< Maximum contraction limit, in percentage ( 1.00 = 100% )
    float long_bound;                 //!< Maximum extension limit, in percentage ( 1.00 = 100% )
    float precompression;             //!< Changes compression or extension of the suspension when the truck spawns. This can be used to "level" the suspension of a truck if it sags in game. The default value is 1.0.  
    int options;             //!< Bit flags.
};

/// Section 'ties'
struct Tie
{
    static const char OPTION_n_FILLER = 'n';
    static const char OPTION_v_FILLER = 'v';
    static const char OPTION_i_INVISIBLE = 'i';
    static const char OPTION_s_NO_SELF_LOCK = 's';

    Node::Ref root_node;
    float max_reach_length = 0.f;
    float auto_shorten_rate = 0.f;
    float min_length = 0.f;
    float max_length = 0.f;
    bool is_invisible = false;
    bool disable_self_lock = false;
    float max_stress = DEFAULT_TIE_MAX_STRESS;
    int group = -1;
};

/// Section 'triggers'
struct Trigger
{
    struct EngineTrigger
    {
        enum Function
        {
            ENGINE_TRIGGER_FUNCTION_CLUTCH      = 0,
            ENGINE_TRIGGER_FUNCTION_BRAKE       = 1,
            ENGINE_TRIGGER_FUNCTION_ACCELERATOR = 2,
            ENGINE_TRIGGER_FUNCTION_RPM_CONTROL = 3,
            ENGINE_TRIGGER_FUNCTION_SHIFT_UP    = 4, //!< Do not mix with OPTION_t_CONTINUOUS
            ENGINE_TRIGGER_FUNCTION_SHIFT_DOWN  = 5, //!< Do not mix with OPTION_t_CONTINUOUS

            ENGINE_TRIGGER_FUNCTION_INVALID     = 0xFFFFFFFF
        };

        Function function;
        unsigned int motor_index;
    };

    struct CommandKeyTrigger
    {
        unsigned int contraction_trigger_key;
        unsigned int extension_trigger_key;
    };

    struct HookToggleTrigger
    {
        int contraction_trigger_hookgroup_id;
        int extension_trigger_hookgroup_id;
    };

    Trigger();

    BITMASK_PROPERTY(options,  1, OPTION_i_INVISIBLE             , HasFlag_i_Invisible,         SetFlag_i_Invisible         )
    BITMASK_PROPERTY(options,  2, OPTION_c_COMMAND_STYLE         , HasFlag_c_CommandStyle,      SetFlag_c_CommandStyle      )
    BITMASK_PROPERTY(options,  3, OPTION_x_START_OFF             , HasFlag_x_StartDisabled,     SetFlag_x_StartDisabled     )
    BITMASK_PROPERTY(options,  4, OPTION_b_BLOCK_KEYS            , HasFlag_b_KeyBlocker,        SetFlag_b_KeyBlocker        )
    BITMASK_PROPERTY(options,  5, OPTION_B_BLOCK_TRIGGERS        , HasFlag_B_TriggerBlocker,    SetFlag_B_TriggerBlocker    )
    BITMASK_PROPERTY(options,  6, OPTION_A_INV_BLOCK_TRIGGERS    , HasFlag_A_InvTriggerBlocker, SetFlag_A_InvTriggerBlocker )
    BITMASK_PROPERTY(options,  7, OPTION_s_SWITCH_CMD_NUM        , HasFlag_s_CmdNumSwitch,      SetFlag_s_CmdNumSwitch      )
    BITMASK_PROPERTY(options,  8, OPTION_h_UNLOCK_HOOKGROUPS_KEY , HasFlag_h_UnlocksHookGroup,  SetFlag_h_UnlocksHookGroup  )
    BITMASK_PROPERTY(options,  9, OPTION_H_LOCK_HOOKGROUPS_KEY   , HasFlag_H_LocksHookGroup,    SetFlag_H_LocksHookGroup    )
    BITMASK_PROPERTY(options, 10, OPTION_t_CONTINUOUS            , HasFlag_t_Continuous,        SetFlag_t_Continuous        )
    BITMASK_PROPERTY(options, 11, OPTION_E_ENGINE_TRIGGER        , HasFlag_E_EngineTrigger,     SetFlag_E_EngineTrigger     )

    inline bool IsHookToggleTrigger() { return HasFlag_H_LocksHookGroup() || HasFlag_h_UnlocksHookGroup(); }

    inline bool IsTriggerBlockerAnyType() { return HasFlag_B_TriggerBlocker() || HasFlag_A_InvTriggerBlocker(); }

    inline void SetEngineTrigger(Trigger::EngineTrigger const & trig)
    {
        shortbound_trigger_action = (int) trig.function;
        longbound_trigger_action = (int) trig.motor_index;
    }

    inline Trigger::EngineTrigger GetEngineTrigger() const
    {
        ROR_ASSERT(HasFlag_E_EngineTrigger());
        EngineTrigger trig;
        trig.function = static_cast<EngineTrigger::Function>(shortbound_trigger_action);
        trig.motor_index = static_cast<unsigned int>(longbound_trigger_action);
        return trig;
    }

    inline void SetCommandKeyTrigger(CommandKeyTrigger const & trig)
    {
        shortbound_trigger_action = (int) trig.contraction_trigger_key;
        longbound_trigger_action = (int) trig.extension_trigger_key;
    }

    inline CommandKeyTrigger GetCommandKeyTrigger() const
    {
        ROR_ASSERT(BITMASK_IS_0(options, OPTION_B_BLOCK_TRIGGERS | OPTION_A_INV_BLOCK_TRIGGERS 
            | OPTION_h_UNLOCK_HOOKGROUPS_KEY | OPTION_H_LOCK_HOOKGROUPS_KEY | OPTION_E_ENGINE_TRIGGER));
        CommandKeyTrigger out;
        out.contraction_trigger_key = static_cast<unsigned int>(shortbound_trigger_action);
        out.extension_trigger_key = static_cast<unsigned int>(longbound_trigger_action);
        return out;
    }

    inline void SetHookToggleTrigger(HookToggleTrigger const & trig)
    {
        shortbound_trigger_action = trig.contraction_trigger_hookgroup_id;
        longbound_trigger_action = trig.extension_trigger_hookgroup_id;
    }

    inline HookToggleTrigger GetHookToggleTrigger() const
    {
        ROR_ASSERT(HasFlag_h_UnlocksHookGroup() || HasFlag_H_LocksHookGroup());
        HookToggleTrigger trig;
        trig.contraction_trigger_hookgroup_id = shortbound_trigger_action;
        trig.extension_trigger_hookgroup_id = longbound_trigger_action;
        return trig;
    }

    Node::Ref nodes[2];
    float contraction_trigger_limit;
    float expansion_trigger_limit;
    unsigned int options;
    float boundary_timer;
    int shortbound_trigger_action;
    int longbound_trigger_action;
};

} // namespace Truck
} // namespace RoR

