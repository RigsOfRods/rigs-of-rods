/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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
/// @date   04/2015

#pragma once

#include "BitFlags.h"

#include "RigDef_Prerequisites.h"

#include <memory>
#include <OgreVector3.h>
#include <string>

namespace RigDef
{

struct Node
{
    /// Abstract node ID (numbered or named)
    /// Node name is always available. For numbered nodes, it's the number converted to string.
    /// Node number is only available for explicitly numbered nodes (legacy).
    class Id
    {
    public:
        BITMASK_PROPERTY_GET (m_flags,  1, IS_VALID,         IsValid);
        BITMASK_PROPERTY_GET (m_flags,  2, IS_TYPE_NUMBERED, IsTypeNumbered);
        BITMASK_PROPERTY_GET (m_flags,  3, IS_TYPE_NAMED,    IsTypeNamed);

        // Constructors
        Id();
        Id(unsigned int id_num);
        Id(std::string const & id_str);

        // Setters
        void SetNum(unsigned int id_num);
        void SetStr(std::string const & id_str);

        // Getters
        inline std::string const & Str() const { return m_id_str; }
        inline unsigned int        Num() const { return m_id_num; }

        // Util
        void Invalidate();
        std::string ToString() const;

    private:

        unsigned int m_id_num;
        std::string  m_id_str;
        unsigned int m_flags;
    };

    /// Legacy parser resolved references on-the-fly and the condition to check named nodes was "are there any named nodes defined at this point?"
    /// The new parser defers node resolution, so every ref. must keep track whether named nodes were already defined at that point.
    class Ref
    {
    public:
        // Since fileformatversion is not known from the beginning of parsing, 2 states must be kept 
        // at the same time: IMPORT_STATE and REGULAR_STATE. The outer logic must make the right pick.
        BITMASK_PROPERTY     (m_flags,  1, IMPORT_STATE_IS_VALID,               GetImportState_IsValid,            SetImportState_IsValid);
        BITMASK_PROPERTY_GET (m_flags,  2, IMPORT_STATE_MUST_CHECK_NAMED_FIRST, GetImportState_MustCheckNamedFirst);
        BITMASK_PROPERTY_GET (m_flags,  3, IMPORT_STATE_IS_RESOLVED_NAMED,      GetImportState_IsResolvedNamed);
        BITMASK_PROPERTY_GET (m_flags,  4, IMPORT_STATE_IS_RESOLVED_NUMBERED,   GetImportState_IsResolvedNumbered);

        BITMASK_PROPERTY     (m_flags,  5, REGULAR_STATE_IS_VALID,            GetRegularState_IsValid,           SetRegularState_IsValid);
        BITMASK_PROPERTY_GET (m_flags,  6, REGULAR_STATE_IS_NAMED,            GetRegularState_IsNamed);
        BITMASK_PROPERTY_GET (m_flags,  7, REGULAR_STATE_IS_NUMBERED,         GetRegularState_IsNumbered);

        Ref(std::string const & id_str, unsigned int id_num, unsigned flags, unsigned line_number_defined);
        Ref();

        inline std::string const & Str() const        { return m_id; }
        inline unsigned int        Num() const        { return m_id_as_number; }

        inline bool Compare   (Ref const & rhs) const { return m_id == rhs.m_id; }
        inline bool operator==(Ref const & rhs) const { return Compare(rhs); }
        inline bool operator!=(Ref const & rhs) const { return ! Compare(rhs); }

        inline bool     IsValidAnyState() const       { return GetImportState_IsValid() || GetRegularState_IsValid(); }
        inline unsigned GetLineNumber() const         { return m_line_number; }

        void Invalidate();
        std::string ToString() const;

    private:
        std::string  m_id;
        unsigned int m_id_as_number;
        unsigned int m_flags;
        unsigned int m_line_number;
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

        inline bool IsRange() const { return start != end; }

        void SetSingle(Node::Ref const & ref)
        {
            start = ref;
            end = ref;
        }

        Node::Ref start;
        Node::Ref end;
    };

    Node():
        position(Ogre::Vector3::ZERO),
        options(0),
        load_weight_override(0),
        _has_load_weight_override(false),
        detacher_group(0), /* Global detacher group */
        editor_group_id(-1)
    {}

    BITMASK_PROPERTY( options,  1, OPTION_n_MOUSE_GRAB        , HasFlag_n, SetFlag_n)
    BITMASK_PROPERTY( options,  2, OPTION_m_NO_MOUSE_GRAB     , HasFlag_m, SetFlag_m)
    BITMASK_PROPERTY( options,  3, OPTION_f_NO_SPARKS         , HasFlag_f, SetFlag_f)
    BITMASK_PROPERTY( options,  4, OPTION_x_EXHAUST_POINT     , HasFlag_x, SetFlag_x)
    BITMASK_PROPERTY( options,  5, OPTION_y_EXHAUST_DIRECTION , HasFlag_y, SetFlag_y)
    BITMASK_PROPERTY( options,  6, OPTION_c_NO_GROUND_CONTACT , HasFlag_c, SetFlag_c)
    BITMASK_PROPERTY( options,  7, OPTION_h_HOOK_POINT        , HasFlag_h, SetFlag_h)
    BITMASK_PROPERTY( options,  8, OPTION_e_TERRAIN_EDIT_POINT, HasFlag_e, SetFlag_e)
    BITMASK_PROPERTY( options,  9, OPTION_b_EXTRA_BUOYANCY    , HasFlag_b, SetFlag_b)
    BITMASK_PROPERTY( options, 10, OPTION_p_NO_PARTICLES      , HasFlag_p, SetFlag_p)
    BITMASK_PROPERTY( options, 11, OPTION_L_LOG               , HasFlag_L, SetFlag_L)
    BITMASK_PROPERTY( options, 12, OPTION_l_LOAD_WEIGHT       , HasFlag_l, SetFlag_l)

    Id id;
    Ogre::Vector3 position;
    unsigned int options; ///< Bit flags
    float load_weight_override;
    bool _has_load_weight_override;
    std::shared_ptr<NodeDefaults> node_defaults;
    std::shared_ptr<MinimassPreset> node_minimass;
    std::shared_ptr<BeamDefaults> beam_defaults; /* Needed for hook */
    int detacher_group;
    int editor_group_id;
};

} //namespace RigDef
