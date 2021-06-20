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

#include "DashLampOverlayElement.h"

#include "Console.h"

#include <fmt/format.h>
#include <OgreTextAreaOverlayElement.h>
#include <Ogre.h>

using namespace RoR;
using namespace Ogre;

// --------------------------------
// The element

    // Static members

const String DashLampOverlayElement::OVERLAY_ELEMENT_TYPE_NAME("DashLamp");

    // Construction/destruction

DashLampOverlayElement::DashLampOverlayElement(const String& name)
    : Ogre::PanelOverlayElement(name)
{
    // Register this class in OGRE parameter system, see Ogre::StringInterface.
    this->createParamDictionary(OVERLAY_ELEMENT_TYPE_NAME);
    // Add Ogre::TextAreaOverlayElement (and ancestor classes) parameters.
    this->addBaseParameters();
    // Add Rigs of Rods dashboard parameters.
    this->addCommonDashParams(this->getParamDictionary());

    this->locateMaterials();
}

    // Functions

const Ogre::String& DashLampOverlayElement::getTypeName(void) const
{
    return OVERLAY_ELEMENT_TYPE_NAME;
}
/*
  //  Severity	Code	Description	Project	File	Line	Suppression State
  //  Error	LNK2005	"public: virtual void __cdecl RoR::BaseDashOverlayElement::CmdLink::doSet(void *,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)" (?doSet@CmdLink@BaseDashOverlayElement@RoR@@UEAAXPEAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z) already defined in DashTextAreaOverlayElement.obj	RoR	C:\Users\Petr\builds\rigs-of-rods\source\main\DashLampOverlayElement.obj	1	
  //  Error	LNK2005	"public: virtual class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __cdecl RoR::BaseDashOverlayElement::CmdAnim::doGet(void const *)const " (?doGet@CmdAnim@BaseDashOverlayElement@RoR@@UEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBX@Z) already defined in DashTextAreaOverlayElement.obj	RoR	C:\Users\Petr\builds\rigs-of-rods\source\main\DashLampOverlayElement.obj	1	
  //  Error	LNK2005	"public: virtual class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __cdecl RoR::BaseDashOverlayElement::CmdLink::doGet(void const *)const " (?doGet@CmdLink@BaseDashOverlayElement@RoR@@UEBA?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEBX@Z) already defined in DashTextAreaOverlayElement.obj	RoR	C:\Users\Petr\builds\rigs-of-rods\source\main\DashLampOverlayElement.obj	1	
  //  Error	LNK2005	"public: virtual void __cdecl RoR::BaseDashOverlayElement::CmdAnim::doSet(void *,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)" (?doSet@CmdAnim@BaseDashOverlayElement@RoR@@UEAAXPEAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z) already defined in DashTextAreaOverlayElement.obj	RoR	C:\Users\Petr\builds\rigs-of-rods\source\main\DashLampOverlayElement.obj	1	

String DashLampOverlayElement::CmdAnim::doGet( const void* target ) const
{
    return static_cast<const DashLampOverlayElement*>(target)->getAnimStr();
}
String DashLampOverlayElement::CmdLink::doGet( const void* target ) const
{
    return static_cast<const DashLampOverlayElement*>(target)->getLinkStr();
}

void DashLampOverlayElement::CmdAnim::doSet( void* target, const String& val )
{
    DashLampOverlayElement* obj = static_cast<DashLampOverlayElement*>(target);
    obj->setAnimStr(val);
}
void DashLampOverlayElement::CmdLink::doSet( void* target, const String& val )
{
    DashLampOverlayElement* obj = static_cast<DashLampOverlayElement*>(target);
    obj->setLinkStr(val);
}*/

// Type-specific functions

void DashLampOverlayElement::locateMaterials()
{
    /// Materials must have suffix "-on" and "-off". One must be specified in overlay script - the other will be deduced.
    if (Ogre::StringUtil::endsWith(this->getMaterial()->getName(), "-on"))
    {
        m_on_material = this->getMaterial();
        Ogre::String base_name = this->getMaterial()->getName().substr(0, this->getMaterial()->getName().length() - 3);
        m_off_material = Ogre::MaterialManager::getSingleton().getByName(base_name + "-off");
    }
    else if (Ogre::StringUtil::endsWith(this->getMaterial()->getName(), "-off"))
    {
        m_off_material = this->getMaterial();
        Ogre::String base_name = this->getMaterial()->getName().substr(0, this->getMaterial()->getName().length() - 4);
        m_on_material = Ogre::MaterialManager::getSingleton().getByName(base_name + "-on");
    }
    else
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
            fmt::format("Unrecognized material '{}' - lamp will not be animated", this->getMaterial()->getName()));
    }
}

bool DashLampOverlayElement::checkMaterialsOk() const
{
    return m_on_material && m_off_material;
}

void DashLampOverlayElement::setLampOn(bool on)
{
    this->setMaterial(on ? m_on_material : m_off_material);
}

// --------------------------------
// The factory

Ogre::OverlayElement* DashLampOverlayElementFactory::createOverlayElement(const Ogre::String& instanceName)
{
    return OGRE_NEW DashLampOverlayElement(instanceName);
}

const Ogre::String& DashLampOverlayElementFactory::getTypeName() const
{
    return DashLampOverlayElement::OVERLAY_ELEMENT_TYPE_NAME;
}

