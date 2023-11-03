/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#include "Application.h"
#include "GenericFileFormat.h"
#include "RefCountingObject.h"
#include "RigDef_File.h" // Document

#include <OgreResourceManager.h>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace RoR {

/// NOTE: Modcache processes this format directly using `RoR::GenericDocument`, see `RoR::CacheSystem::FillAddonPartDetailInfo()`
class AddonPartUtility
{
public:
    /// transforms the addonpart to `RigDef::File::Module` (fake 'section/end_section') used for spawning.
    /// @return nullptr on error
    std::shared_ptr<RigDef::Document::Module> TransformToRigDefModule(CacheEntryPtr& addonpart_entry);

    /// Evaluates 'addonpart_unwanted_*' elements, respecting 'protected_*' directives in the tuneup.
    void ResolveUnwantedElements(TuneupDefPtr& tuneup, CacheEntryPtr& addonpart_entry);

private:
    // Helpers of `TransformToRigDefModule()`, they expect `m_context` to be in position:
    void ProcessManagedMaterial();
    void ProcessProp();
    void ProcessFlexbody();

    // SHARED CONTEXT
    GenericDocumentPtr m_document;
    GenericDocContextPtr m_context;
    std::shared_ptr<RigDef::Document::Module> m_module;
    CacheEntryPtr m_addonpart_entry;
};

}; // namespace RoR
