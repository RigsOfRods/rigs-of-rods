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

/// @file
/// @author Petr Ohlidal, 2021
/// @brief Common logic for all dashboard overlay elements.

/// OGRE's overlay system is designed to be extensible, but the mechanism is quite complex.
/// To add new parameter 'foo' you must:
///   1. subclass FooCmd from ParamCommand and define doGet/doSet
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

/// dashboard extension parameters for OGRE overlay elements
/// Think of it as "partial class" or "mixin class" in other languages.
class BaseDashOverlayElement
{
public:
    virtual ~BaseDashOverlayElement() {};

    Ogre::String getAnimStr() const { return m_anim_str; }
    void setAnimStr(Ogre::String const& str) { m_anim_str = str; }

    Ogre::String getLinkStr() const { return m_link_str; }
    void setLinkStr(Ogre::String const& str) { m_link_str = str; }

protected:

    // OGRE overlay param extension mechanism, step 2
    static CmdAnim ms_anim_cmd;
    static CmdLink ms_link_cmd;

    // OGRE overlay param extension mechanism, step 3
    /// @param dict must be provied by the Ogre::OverlayElement being extended.
    void addCommonDashParams(Ogre::ParamDictionary* dict);

    // Variables
    Ogre::String m_anim_str; //!< See ANIM_*
    Ogre::String m_link_str; //!< See DD_*
};

} // namespace RoR
