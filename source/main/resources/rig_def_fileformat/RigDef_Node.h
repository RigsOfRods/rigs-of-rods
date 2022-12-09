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
/// @date   04/2015

#pragma once

#include "BitFlags.h"

#include "RigDef_Prerequisites.h"

#include <memory>
#include <Ogre.h>
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
        void setStr(std::string const & id_str);

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
        BITMASK_PROPERTY_GET (m_flags,  1, IMPORT_STATE_IS_VALID,               GetImportState_IsValid);
        BITMASK_PROPERTY_GET (m_flags,  2, IMPORT_STATE_MUST_CHECK_NAMED_FIRST, GetImportState_MustCheckNamedFirst);
        BITMASK_PROPERTY_GET (m_flags,  3, IMPORT_STATE_IS_RESOLVED_NAMED,      GetImportState_IsResolvedNamed);
        BITMASK_PROPERTY_GET (m_flags,  4, IMPORT_STATE_IS_RESOLVED_NUMBERED,   GetImportState_IsResolvedNumbered);

        BITMASK_PROPERTY_GET (m_flags,  5, REGULAR_STATE_IS_VALID,            GetRegularState_IsValid);
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
        detacher_group(0) /* Global detacher group */
    {}

    static const BitMask_t OPTION_m_NO_MOUSE_GRAB      = BITMASK(1);
    static const BitMask_t OPTION_f_NO_SPARKS          = BITMASK(2);
    static const BitMask_t OPTION_x_EXHAUST_POINT      = BITMASK(3);
    static const BitMask_t OPTION_y_EXHAUST_DIRECTION  = BITMASK(4);
    static const BitMask_t OPTION_c_NO_GROUND_CONTACT  = BITMASK(5);
    static const BitMask_t OPTION_h_HOOK_POINT         = BITMASK(6);
    static const BitMask_t OPTION_e_TERRAIN_EDIT_POINT = BITMASK(7);
    static const BitMask_t OPTION_b_EXTRA_BUOYANCY     = BITMASK(8);
    static const BitMask_t OPTION_p_NO_PARTICLES       = BITMASK(9);
    static const BitMask_t OPTION_L_LOG                = BITMASK(10);
    static const BitMask_t OPTION_l_LOAD_WEIGHT        = BITMASK(11);

    Id id;
    Ogre::Vector3 position;
    BitMask_t options;
    float load_weight_override;
    bool _has_load_weight_override;
    std::shared_ptr<NodeDefaults> node_defaults;
    std::shared_ptr<DefaultMinimass> default_minimass; // override of global 'minimass'.
    std::shared_ptr<BeamDefaults> beam_defaults; /* Needed for hook */
    int detacher_group;
};

} //namespace RigDef
