/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

/** 
    @file   
    @date   12/2014
    @author Petr Ohlidal

    AggregateData structures provide a connection between rig elements and associated GUI panels.
    Usage:
        * Query info about selected elements in rig.
        * Provide data backend for GUI panel associated with the element.
        * Bulk update of selected elements in rig.

    Contents of *AggregateData structure:
        * Element count
        * Data fields (element-specific)
        * Uniformity flags (1 per each data field).
          - TRUE if the field's value is uniform across all aggregated elements, FALSE if not.
*/

#pragma once

#include <cstdint>
#include <OgreAxisAlignedBox.h>

#include "RigDef_File.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_Types.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

struct RigAggregateNodesData
{
    RigAggregateNodesData()
    {
        Reset();
    }

    inline unsigned int* GetFlagsPtr() { return & m_query_flags; }

    void Reset()
    {
        num_selected = 0;
        load_weight = 0.f;
        detacher_group_id = 0;
        preset_id = 0;
        m_query_flags = BITMASK_FILL_1_EXCEPT(
            VALUE_FLAG_m|VALUE_FLAG_l|VALUE_FLAG_p|VALUE_FLAG_b|VALUE_FLAG_h|
            VALUE_FLAG_c|VALUE_FLAG_y|VALUE_FLAG_x|VALUE_FLAG_f|VALUE_FLAG_L);
    }

    BITMASK_PROPERTY(m_query_flags,  1, QUERY_LOAD_WEIGHT_IS_UNIFORM, IsLoadWeightUniform, SetLoadWeightIsUniform)
    BITMASK_PROPERTY(m_query_flags,  2, QUERY_DETACHER_GROUP_IS_UNIFORM, IsDetacherGroupUniform, SetDetacherGroupIsUniform)
    BITMASK_PROPERTY(m_query_flags,  3, QUERY_PRESET_IS_UNIFORM, IsPresetUniform, SetPresetIsUniform)

    BITMASK_PROPERTY(m_query_flags,  4, QUERY_FLAG_m_IS_UNIFORM, IsFlagUniform_m, SetFlagIsUniform_m)
    BITMASK_PROPERTY(m_query_flags,  5, QUERY_FLAG_l_IS_UNIFORM, IsFlagUniform_l, SetFlagIsUniform_l)
    BITMASK_PROPERTY(m_query_flags,  6, QUERY_FLAG_p_IS_UNIFORM, IsFlagUniform_p, SetFlagIsUniform_p)
    BITMASK_PROPERTY(m_query_flags,  7, QUERY_FLAG_b_IS_UNIFORM, IsFlagUniform_b, SetFlagIsUniform_b)
    BITMASK_PROPERTY(m_query_flags,  8, QUERY_FLAG_h_IS_UNIFORM, IsFlagUniform_h, SetFlagIsUniform_h)
    BITMASK_PROPERTY(m_query_flags,  9, QUERY_FLAG_c_IS_UNIFORM, IsFlagUniform_c, SetFlagIsUniform_c)
    BITMASK_PROPERTY(m_query_flags, 10, QUERY_FLAG_y_IS_UNIFORM, IsFlagUniform_y, SetFlagIsUniform_y)
    BITMASK_PROPERTY(m_query_flags, 11, QUERY_FLAG_x_IS_UNIFORM, IsFlagUniform_x, SetFlagIsUniform_x)
    BITMASK_PROPERTY(m_query_flags, 12, QUERY_FLAG_f_IS_UNIFORM, IsFlagUniform_f, SetFlagIsUniform_f)
    BITMASK_PROPERTY(m_query_flags, 13, QUERY_FLAG_L_IS_UNIFORM, IsFlagUniform_L, SetFlagIsUniform_L)

    BITMASK_PROPERTY(m_query_flags, 14, VALUE_FLAG_m, HasFlag_m, SetFlag_m)
    BITMASK_PROPERTY(m_query_flags, 15, VALUE_FLAG_l, HasFlag_l, SetFlag_l)
    BITMASK_PROPERTY(m_query_flags, 16, VALUE_FLAG_p, HasFlag_p, SetFlag_p)
    BITMASK_PROPERTY(m_query_flags, 17, VALUE_FLAG_b, HasFlag_b, SetFlag_b)
    BITMASK_PROPERTY(m_query_flags, 18, VALUE_FLAG_h, HasFlag_h, SetFlag_h)
    BITMASK_PROPERTY(m_query_flags, 19, VALUE_FLAG_c, HasFlag_c, SetFlag_c)
    BITMASK_PROPERTY(m_query_flags, 20, VALUE_FLAG_y, HasFlag_y, SetFlag_y)
    BITMASK_PROPERTY(m_query_flags, 21, VALUE_FLAG_x, HasFlag_x, SetFlag_x)
    BITMASK_PROPERTY(m_query_flags, 22, VALUE_FLAG_f, HasFlag_f, SetFlag_f)
    BITMASK_PROPERTY(m_query_flags, 23, VALUE_FLAG_L, HasFlag_L, SetFlag_L)

    static const unsigned int RESERVED_NODENAME_UNIFORM = BITMASK(24);
    
    int          num_selected;
    Ogre::String node_name;
    float        load_weight;
    int          detacher_group_id;
    int          preset_id;

private:
    unsigned int m_query_flags;
};

struct RigAggregatePlainBeamsData
{
    RigAggregatePlainBeamsData()
    {
        Reset();
    }

    inline unsigned int* GetFlagsPtr() { return &m_query_flags; }

    void Reset()
    {
        num_selected = 0;
        extension_break_limit = 0.f;
        detacher_group = 0;
        m_query_flags = (BITMASK_FILL_1_EXCEPT(VALUE_FLAG_i|VALUE_FLAG_s|VALUE_FLAG_r));
    }

    BITMASK_PROPERTY(m_query_flags, 1, VALUE_FLAG_i, HasFlag_i, SetFlag_i)
    BITMASK_PROPERTY(m_query_flags, 2, VALUE_FLAG_r, HasFlag_r, SetFlag_r)
    BITMASK_PROPERTY(m_query_flags, 3, VALUE_FLAG_s, HasFlag_s, SetFlag_s)

    BITMASK_PROPERTY(m_query_flags, 4, QUERY_FLAG_i_IS_UNIFORM, IsFlagUniform_i, SetFlagUniform_i)
    BITMASK_PROPERTY(m_query_flags, 5, QUERY_FLAG_r_IS_UNIFORM, IsFlagUniform_r, SetFlagUniform_r)
    BITMASK_PROPERTY(m_query_flags, 6, QUERY_FLAG_s_IS_UNIFORM, IsFlagUniform_s, SetFlagUniform_s)

    BITMASK_PROPERTY(m_query_flags, 7, QUERY_EXTENSION_BREAKLIMIT_IS_UNIFORM, IsExtensionBreakLimitUniform, SetExtensionBreakLimitIsUniform)
    BITMASK_PROPERTY(m_query_flags, 8, QUERY_DETACHER_GROUP_IS_UNIFORM,       IsDetacherGroupUniform,       SetDetacherGroupIsUniform)

    int num_selected;
    float extension_break_limit;
    int detacher_group;
private:
    unsigned int m_query_flags;
};

struct RigAggregateHydrosData
{
    RigAggregateHydrosData()
    {
        Reset();
    }

    inline unsigned int* GetFlagsPtr() { return &m_query_flags; }

    void Reset()
    {
        memset(this, 0, sizeof(RigAggregateHydrosData));
        m_query_flags = 0x000FFFFF; // Bits 1-20 are 1, rest is 0
    }

    // Bit flags, default 1

    BITMASK_PROPERTY(m_query_flags,  1, QUERY_FLAG_i_IS_UNIFORM, IsFlagUniform_i, SetFlagIsUniform_i)
    BITMASK_PROPERTY(m_query_flags,  2, QUERY_FLAG_s_IS_UNIFORM, IsFlagUniform_s, SetFlagIsUniform_s)
    BITMASK_PROPERTY(m_query_flags,  3, QUERY_FLAG_a_IS_UNIFORM, IsFlagUniform_a, SetFlagIsUniform_a)
    BITMASK_PROPERTY(m_query_flags,  4, QUERY_FLAG_r_IS_UNIFORM, IsFlagUniform_r, SetFlagIsUniform_r)
    BITMASK_PROPERTY(m_query_flags,  5, QUERY_FLAG_e_IS_UNIFORM, IsFlagUniform_e, SetFlagIsUniform_e)
    BITMASK_PROPERTY(m_query_flags,  6, QUERY_FLAG_u_IS_UNIFORM, IsFlagUniform_u, SetFlagIsUniform_u)
    BITMASK_PROPERTY(m_query_flags,  7, QUERY_FLAG_v_IS_UNIFORM, IsFlagUniform_v, SetFlagIsUniform_v)
    BITMASK_PROPERTY(m_query_flags,  8, QUERY_FLAG_x_IS_UNIFORM, IsFlagUniform_x, SetFlagIsUniform_x)
    BITMASK_PROPERTY(m_query_flags,  9, QUERY_FLAG_y_IS_UNIFORM, IsFlagUniform_y, SetFlagIsUniform_y)
    BITMASK_PROPERTY(m_query_flags, 10, QUERY_FLAG_g_IS_UNIFORM, IsFlagUniform_g, SetFlagIsUniform_g)
    BITMASK_PROPERTY(m_query_flags, 11, QUERY_FLAG_h_IS_UNIFORM, IsFlagUniform_h, SetFlagIsUniform_h)

    BITMASK_PROPERTY(m_query_flags, 12, QUERY_EXTENSION_FACTOR_IS_UNIFORM       , IsExtensionFactorUniform,       SetExtensionFactorIsUniform)
    BITMASK_PROPERTY(m_query_flags, 13, QUERY_DETACHER_GROUP_IS_UNIFORM         , IsDetacherGroupUniform,         SetDetacherGroupIsUniform)

    BITMASK_PROPERTY(m_query_flags, 14, QUERY_INERTIA_START_DELAY_IS_UNIFORM    , IsInertiaStartDelayUniform,     SetInertiaStartDelayIsUniform)
    BITMASK_PROPERTY(m_query_flags, 15, QUERY_INERTIA_STOP_DELAY_IS_UNIFORM     , IsInertiaStopDelayUniform,      SetInertiaStopDelayIsUniform)
    BITMASK_PROPERTY(m_query_flags, 16, QUERY_INERTIA_START_FUNCTION_IS_UNIFORM , IsInertiaStartFunctionUniform,  SetInertiaStartFunctionIsUniform)
    BITMASK_PROPERTY(m_query_flags, 17, QUERY_INERTIA_STOP_FUNCTION_IS_UNIFORM  , IsInertiaStopFunctionUniform,   SetInertiaStopFunctionIsUniform)

    // Bit flags, default 0
    BITMASK_PROPERTY(m_query_flags, 22, VALUE_FLAG_i, HasFlag_i, SetFlag_i)
    BITMASK_PROPERTY(m_query_flags, 23, VALUE_FLAG_s, HasFlag_s, SetFlag_s)
    BITMASK_PROPERTY(m_query_flags, 24, VALUE_FLAG_a, HasFlag_a, SetFlag_a)
    BITMASK_PROPERTY(m_query_flags, 25, VALUE_FLAG_r, HasFlag_r, SetFlag_r)
    BITMASK_PROPERTY(m_query_flags, 26, VALUE_FLAG_e, HasFlag_e, SetFlag_e)
    BITMASK_PROPERTY(m_query_flags, 27, VALUE_FLAG_u, HasFlag_u, SetFlag_u)
    BITMASK_PROPERTY(m_query_flags, 28, VALUE_FLAG_v, HasFlag_v, SetFlag_v)
    BITMASK_PROPERTY(m_query_flags, 29, VALUE_FLAG_x, HasFlag_x, SetFlag_x)
    BITMASK_PROPERTY(m_query_flags, 30, VALUE_FLAG_y, HasFlag_y, SetFlag_y)
    BITMASK_PROPERTY(m_query_flags, 31, VALUE_FLAG_g, HasFlag_g, SetFlag_g)
    BITMASK_PROPERTY(m_query_flags, 32, VALUE_FLAG_h, HasFlag_h, SetFlag_h)

    int num_selected;
    float extension_factor;
    float inertia_start_delay;
    float inertia_stop_delay;
    Ogre::String inertia_stop_function;
    Ogre::String inertia_start_function;
    int detacher_group;
private:
    unsigned int m_query_flags;
};

struct RigAggregateShocksData
{
    RigAggregateShocksData()
    {
        Reset();
    }

    inline void Reset()
    {
        memset(this, 0, sizeof(RigAggregateShocksData));
        m_query_flags = BITMASK_FILL_1_EXCEPT(VALUE_FLAG_i|VALUE_FLAG_R|VALUE_FLAG_L|VALUE_FLAG_m);
    }

    inline unsigned int* GetFlagsPtr() { return &m_query_flags; }

    BITMASK_PROPERTY(m_query_flags,  1, QUERY_SPRING_IS_UNIFORM           , IsSpringUniform,           SetSpringIsUniform)
    BITMASK_PROPERTY(m_query_flags,  2, QUERY_DAMPING_IS_UNIFORM          , IsDampingUniform,          SetDampingIsUniform)
    BITMASK_PROPERTY(m_query_flags,  3, QUERY_CONTRACTION_LIMIT_IS_UNIFORM, IsContractionLimitUniform, SetContractionLimitIsUniform)
    BITMASK_PROPERTY(m_query_flags,  4, QUERY_EXTENSION_LIMIT_IS_UNIFORM  , IsExtensionLimitUniform,   SetExtensionLimitIsUniform)
    BITMASK_PROPERTY(m_query_flags,  5, QUERY_PRECOMPRESSION_IS_UNIFORM   , IsPrecompressionUniform,   SetPrecompressionIsUniform)
    BITMASK_PROPERTY(m_query_flags,  6, QUERY_DETACHER_GROUP_IS_UNIFORM   , IsDetacherGroupUniform,    SetDetacherGroupIsUniform)
    BITMASK_PROPERTY(m_query_flags,  7, QUERY_FLAG_i_IS_UNIFORM           , IsFlagUniform_i,           SetFlagIsUniform_i)
    BITMASK_PROPERTY(m_query_flags,  8, QUERY_FLAG_L_IS_UNIFORM           , IsFlagUniform_L,           SetFlagIsUniform_L)
    BITMASK_PROPERTY(m_query_flags,  9, QUERY_FLAG_R_IS_UNIFORM           , IsFlagUniform_R,           SetFlagIsUniform_R)
    BITMASK_PROPERTY(m_query_flags, 10, QUERY_FLAG_m_METRIC               , IsFlagUniform_m,           SetFlagIsUniform_m)

    BITMASK_PROPERTY(m_query_flags, 11, VALUE_FLAG_i, HasFlag_i, SetFlag_i)
    BITMASK_PROPERTY(m_query_flags, 12, VALUE_FLAG_L, HasFlag_L, SetFlag_L)
    BITMASK_PROPERTY(m_query_flags, 13, VALUE_FLAG_R, HasFlag_R, SetFlag_R)
    BITMASK_PROPERTY(m_query_flags, 13, VALUE_FLAG_m, HasFlag_m, SetFlag_m)

    int num_selected;
    
    float spring_rate;
    float damping;
    float contraction_limit;
    float extension_limit;
    float precompression;
    int detacher_group;
private:
    unsigned int m_query_flags;
};

struct RigAggregateShocks2Data
{
    RigAggregateShocks2Data()
    {
        Reset();
    }

    void Reset()
    {
        memset(this, 0, sizeof(RigAggregateShocks2Data));
        m_query_flags = BITMASK_FILL_1_EXCEPT(VALUE_FLAG_i|VALUE_FLAG_s|VALUE_FLAG_m|VALUE_FLAG_M);
    }
    
    BITMASK_PROPERTY(m_query_flags,  1, QUERY_SPRING_IN_RATE_IS_UNIFORM       , IsSpringInRateUniform,      SetSpringInRateIsUniform       )
    BITMASK_PROPERTY(m_query_flags,  2, QUERY_SPRING_IN_PROGRESS_IS_UNIFORM   , IsSpringInProgressUniform,  SetSpringInProgressIsUniform   )
    BITMASK_PROPERTY(m_query_flags,  3, QUERY_SPRING_OUT_RATE_IS_UNIFORM      , IsSpringOutRateUniform,     SetSpringOutRateIsUniform      )
    BITMASK_PROPERTY(m_query_flags,  4, QUERY_SPRING_OUT_PROGRESS_IS_UNIFORM  , IsSpringOutProgressUniform, SetSpringOutProgressIsUniform  )
    BITMASK_PROPERTY(m_query_flags,  5, QUERY_DAMP_IN_RATE_IS_UNIFORM         , IsDampInRateUniform,        SetDampInRateIsUniform         )
    BITMASK_PROPERTY(m_query_flags,  6, QUERY_DAMP_IN_PROGRESS_IS_UNIFORM     , IsDampInProgressUniform,    SetDampInProgressIsUniform     )
    BITMASK_PROPERTY(m_query_flags,  7, QUERY_DAMP_OUT_RATE_IS_UNIFORM        , IsDampOutRateUniform,       SetDampOutRateIsUniform        )
    BITMASK_PROPERTY(m_query_flags,  8, QUERY_DAMP_OUT_PROGRESS_IS_UNIFORM    , IsDampOutProgressUniform,   SetDampOutProgressIsUniform    )
    BITMASK_PROPERTY(m_query_flags,  9, QUERY_CONTRACTION_LIMIT_IS_UNIFORM    , IsContractionLimitUniform,  SetContractionLimitIsUniform   )
    BITMASK_PROPERTY(m_query_flags, 10, QUERY_EXTENSION_LIMIT_IS_UNIFORM      , IsExtensionLimitUniform,    SetExtensionLimitIsUniform     )
    BITMASK_PROPERTY(m_query_flags, 11, QUERY_PRECOMPRESSION_IS_UNIFORM       , IsPrecompressionUniform,    SetPrecompressionIsUniform     )
    BITMASK_PROPERTY(m_query_flags, 12, QUERY_DETACHER_GROUP_IS_UNIFORM       , IsDetacherGroupUniform,     SetDetacherGroupIsUniform      )
    BITMASK_PROPERTY(m_query_flags, 13, QUERY_FLAG_i_IS_UNIFORM               , IsFlagUniform_i,            SetFlagIsUniform_i)
    BITMASK_PROPERTY(m_query_flags, 14, QUERY_FLAG_s_IS_UNIFORM               , IsFlagUniform_s,            SetFlagIsUniform_s)
    BITMASK_PROPERTY(m_query_flags, 15, QUERY_FLAG_m_IS_UNIFORM               , IsFlagUniform_m,            SetFlagIsUniform_m)
    BITMASK_PROPERTY(m_query_flags, 16, QUERY_FLAG_M_IS_UNIFORM               , IsFlagUniform_M,            SetFlagIsUniform_M)

    BITMASK_PROPERTY(m_query_flags, 17, VALUE_FLAG_i                          , HasFlag_i, SetFlag_i)
    BITMASK_PROPERTY(m_query_flags, 18, VALUE_FLAG_s                          , HasFlag_s, SetFlag_s)
    BITMASK_PROPERTY(m_query_flags, 19, VALUE_FLAG_m                          , HasFlag_m, SetFlag_m)
    BITMASK_PROPERTY(m_query_flags, 20, VALUE_FLAG_M                          , HasFlag_M, SetFlag_M)

    inline unsigned int* GetFlagsPtr() { return &m_query_flags; }

    int num_selected;
    float spring_in_rate;
    float spring_out_rate;
    float spring_in_progress;
    float spring_out_progress;
    float damp_in_rate;
    float damp_out_rate;
    float damp_in_progress;
    float damp_out_progress;
    float contraction_limit;
    float extension_limit;
    float precompression;
    int detacher_group;
private:
    unsigned int m_query_flags;
};

struct RigAggregateCommands2Data
{
    RigAggregateCommands2Data()
    {
        Reset();
    }

    void Reset()
    {
        memset(this, 0, sizeof(RigAggregateCommands2Data));
        m_query_flags = BITMASK_FILL_1_EXCEPT(VALUE_NEEDS_ENGINE|VALUE_FLAG_i|VALUE_FLAG_r|VALUE_FLAG_c|VALUE_FLAG_f|VALUE_FLAG_p|VALUE_FLAG_o);
    }

    BITMASK_PROPERTY(m_query_flags,  1, QUERY_DETACHER_GROUP_IS_UNIFORM         , IsDetacherGroupUniform,         SetDetacherGroupIsUniform)
    BITMASK_PROPERTY(m_query_flags,  2, QUERY_CONTRACT_RATE_IS_UNIFORM          , IsContractionRateUniform,       SetContractionRateIsUniform)
    BITMASK_PROPERTY(m_query_flags,  3, QUERY_EXTEND_RATE_IS_UNIFORM            , IsExtensionRateUniform,         SetExtensionRateIsUniform)
    BITMASK_PROPERTY(m_query_flags,  4, QUERY_CONTRACT_LIMIT_IS_UNIFORM         , IsContractionLimitUniform,      SetContractionLimitIsUniform)
    BITMASK_PROPERTY(m_query_flags,  5, QUERY_EXTEND_LIMIT_IS_UNIFORM           , IsExtensionLimitUniform,        SetExtensionLimitIsUniform)
    BITMASK_PROPERTY(m_query_flags,  6, QUERY_CONTRACT_KEY_IS_UNIFORM           , IsContractKeyUniform,           SetContractKeyIsUniform)
    BITMASK_PROPERTY(m_query_flags,  7, QUERY_EXTEND_KEY_IS_UNIFORM             , IsExtendKeyUniform,             SetExtendKeyIsUniform)
    BITMASK_PROPERTY(m_query_flags,  8, QUERY_DESCRIPTION_IS_UNIFORM            , IsDescriptionUniform,           SetDescriptionIsUniform)

    BITMASK_PROPERTY(m_query_flags,  9, QUERY_INERTIA_START_DELAY_IS_UNIFORM    , IsInertiaStartDelayUniform,     SetInertiaStartDelayIsUniform)
    BITMASK_PROPERTY(m_query_flags, 10, QUERY_INERTIA_STOP_DELAY_IS_UNIFORM     , IsInertiaStopDelayUniform,      SetInertiaStopDelayIsUniform)
    BITMASK_PROPERTY(m_query_flags, 11, QUERY_INERTIA_START_FUNCTION_IS_UNIFORM , IsInertiaStartFunctionUniform,  SetInertiaStartFunctionIsUniform)
    BITMASK_PROPERTY(m_query_flags, 12, QUERY_INERTIA_STOP_FUNCTION_IS_UNIFORM  , IsInertiaStopFunctionUniform,   SetInertiaStopFunctionIsUniform)

    BITMASK_PROPERTY(m_query_flags, 13, VALUE_NEEDS_ENGINE                      , GetBoolNeedsEngine,             SetBoolNeedsEngine)
    BITMASK_PROPERTY(m_query_flags, 14, QUERY_NEEDS_ENGINE_IS_UNIFORM           , IsNeedsEngineUniform,           SetNeedsEngineIsUniform)
    BITMASK_PROPERTY(m_query_flags, 15, QUERY_AFFECT_ENGINE_IS_UNIFORM          , IsAffectEngineUniform,          SetAffectEngineIsUniform)

    BITMASK_PROPERTY(m_query_flags, 16, VALUE_FLAG_i, HasFlag_i, SetFlag_i)
    BITMASK_PROPERTY(m_query_flags, 17, VALUE_FLAG_r, HasFlag_r, SetFlag_r)
    BITMASK_PROPERTY(m_query_flags, 18, VALUE_FLAG_c, HasFlag_c, SetFlag_c)
    BITMASK_PROPERTY(m_query_flags, 19, VALUE_FLAG_f, HasFlag_f, SetFlag_f)
    BITMASK_PROPERTY(m_query_flags, 20, VALUE_FLAG_p, HasFlag_p, SetFlag_p)
    BITMASK_PROPERTY(m_query_flags, 21, VALUE_FLAG_o, HasFlag_o, SetFlag_o)

    BITMASK_PROPERTY(m_query_flags, 22, QUERY_FLAG_IS_UNIFORM_i, IsFlagUniform_i, SetFlagIsUniform_i)
    BITMASK_PROPERTY(m_query_flags, 23, QUERY_FLAG_IS_UNIFORM_r, IsFlagUniform_r, SetFlagIsUniform_r)
    BITMASK_PROPERTY(m_query_flags, 24, QUERY_FLAG_IS_UNIFORM_c, IsFlagUniform_c, SetFlagIsUniform_c)
    BITMASK_PROPERTY(m_query_flags, 25, QUERY_FLAG_IS_UNIFORM_f, IsFlagUniform_f, SetFlagIsUniform_f)
    BITMASK_PROPERTY(m_query_flags, 26, QUERY_FLAG_IS_UNIFORM_p, IsFlagUniform_p, SetFlagIsUniform_p)
    BITMASK_PROPERTY(m_query_flags, 27, QUERY_FLAG_IS_UNIFORM_o, IsFlagUniform_o, SetFlagIsUniform_o)

    inline unsigned int* GetFlagsPtr() { return &m_query_flags; }

    int num_selected;

    int detacher_group;
    float contraction_rate;
    float extension_rate;
    float max_contraction;
    float max_extension;
    unsigned int contract_key;
    unsigned int extend_key;
    Ogre::String description;
    // Optional inertia
    float inertia_start_delay;
    float inertia_stop_delay;
    Ogre::String inertia_stop_function;
    Ogre::String inertia_start_function;
    
    float affect_engine;
private:
    unsigned int m_query_flags;
};

struct RigAggregateBeams2Data
{
    RigAggregatePlainBeamsData plain_beams;
    RigAggregateHydrosData     hydros;
    RigAggregateCommands2Data  commands2;
    RigAggregateShocksData     shocks;
    RigAggregateShocks2Data    shocks2;

    inline int GetTotalNumSelectedBeams() const 
    {
        return plain_beams.num_selected + hydros.num_selected + commands2.num_selected + shocks.num_selected + shocks2.num_selected;
    }

    inline bool HasMixedBeamTypes() const
    {
        int total = GetTotalNumSelectedBeams();
        return (total != plain_beams.num_selected) && (total != hydros.num_selected) && (total != commands2.num_selected)
            && (total != shocks.num_selected) && (total != shocks2.num_selected);
    }

    bool IsDetacherGroupUniform() 
    {
        bool detacher_group_is_uniform 
            =  plain_beams.IsDetacherGroupUniform() && shocks .IsDetacherGroupUniform()
            && hydros     .IsDetacherGroupUniform() && shocks2.IsDetacherGroupUniform()
            && commands2  .IsDetacherGroupUniform();

        if (detacher_group_is_uniform)
        {
            int detacher_group_id = plain_beams.detacher_group;
            detacher_group_is_uniform = (hydros.detacher_group     != detacher_group_id) ? false : detacher_group_is_uniform;
            detacher_group_is_uniform = (shocks.detacher_group     != detacher_group_id) ? false : detacher_group_is_uniform;
            detacher_group_is_uniform = (shocks2.detacher_group    != detacher_group_id) ? false : detacher_group_is_uniform;
            detacher_group_is_uniform = (commands2.detacher_group  != detacher_group_id) ? false : detacher_group_is_uniform;
        }
        return detacher_group_is_uniform;
    }
};

struct MixedBeamsAggregateData
{
    MixedBeamsAggregateData():
        m_flags(0)
    {}

    BITMASK_PROPERTY(m_flags, 1, DETACHER_GROUP_IS_UNIFORM, IsDetacherGroupUniform, SetDetacherGroupIsUniform)

    int detacher_group;

private:
    unsigned int m_flags;
};
    
} // namespace RigEditor

} // namespace RoR
