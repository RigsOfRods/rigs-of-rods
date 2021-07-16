/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

#pragma once

#include <Ogre.h>
#include <Overlay/OgreOverlayElement.h>

/// @file
/// @author Petr Ohlidal, 2021
/// @brief Common logic for all dashboard overlay elements.

/// OGRE's overlay system is designed to be extensible, but the mechanism is quite complex.
/// To add new parameter 'foo' you must:
///   1. subclass FooCmd from Ogre::ParamCommand and define doGet/doSet
///   2. create static instance of the FooCmd - `ms_foo_cmd`
///   3. create instance of ParameterDef with name "foo" and reference to `ms_foo_cmd`

namespace RoR {

    // OGRE overlay param extension mechanism, step 1

class CmdAnim : public Ogre::ParamCommand
{
public:
    virtual Ogre::String doGet( const void* target ) const override;
    virtual void doSet( void* target, const Ogre::String& val ) override;
};

class CmdLink : public Ogre::ParamCommand
{
public:
    virtual Ogre::String doGet( const void* target ) const override;
    virtual void doSet( void* target, const Ogre::String& val ) override;
};

class CmdTransformMin : public Ogre::ParamCommand
{
public:
    virtual Ogre::String doGet( const void* target ) const override;
    virtual void doSet( void* target, const Ogre::String& val ) override;
};

class CmdTransformMax : public Ogre::ParamCommand
{
public:
    virtual Ogre::String doGet( const void* target ) const override;
    virtual void doSet( void* target, const Ogre::String& val ) override;
};

class CmdInputMin : public Ogre::ParamCommand
{
public:
    virtual Ogre::String doGet( const void* target ) const override;
    virtual void doSet( void* target, const Ogre::String& val ) override;
};

class CmdInputMax : public Ogre::ParamCommand
{
public:
    virtual Ogre::String doGet( const void* target ) const override;
    virtual void doSet( void* target, const Ogre::String& val ) override;
};

/// dashboard extension parameters for OGRE overlay elements
/// Think of it as "partial class" or "mixin class" in other languages.
class BaseDashOverlayElement
{
public:
    virtual ~BaseDashOverlayElement() {};

    static BaseDashOverlayElement* ResolveDashElement(Ogre::OverlayElement* obj);

    Ogre::String getAnimStr() const { return m_anim_str; }
    void setAnimStr(Ogre::String const& str) { m_anim_str = str; }

    Ogre::String getLinkStr() const { return m_link_str; }
    void setLinkStr(Ogre::String const& str) { m_link_str = str; }

    Ogre::Real getTransformMin() const { return m_transform_min; }
    void setTransformMin(Ogre::Real const& str) { m_transform_min = str; }

    Ogre::Real getTransformMax() const { return m_transform_max; }
    void setTransformMax(Ogre::Real const& str) { m_transform_max = str; }

    Ogre::Real getInputMin() const { return m_input_min; }
    void setInputMin(Ogre::Real const& str) { m_input_min = str; }

    Ogre::Real getInputMax() const { return m_input_max; }
    void setInputMax(Ogre::Real const& str) { m_input_max = str; }

protected:

    // OGRE overlay param extension mechanism, step 2
    static CmdAnim          ms_anim_cmd;
    static CmdLink          ms_link_cmd;
    static CmdTransformMin  ms_transform_min_cmd;
    static CmdTransformMax  ms_transform_max_cmd;
    static CmdInputMin      ms_input_min_cmd;
    static CmdInputMax      ms_input_max_cmd;

    // OGRE overlay param extension mechanism, step 3
    /// @param dict must be provied by the Ogre::OverlayElement being extended.
    void addCommonDashParams(Ogre::ParamDictionary* dict);

    // Variables
    Ogre::String m_anim_str; //!< See ANIM_*
    Ogre::String m_link_str; //!< See DD_*
    Ogre::Real m_transform_min;
    Ogre::Real m_transform_max;
    Ogre::Real m_input_min;
    Ogre::Real m_input_max;
};

} // namespace RoR
