/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2018 Petr Ohlidal

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

#include "Character.h"
#include "CharacterFileFormat.h"
#include "Network.h"

#include <memory>

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Character
/// @{

class CharacterFactory
{
public:
    CharacterFactory();
    Character* CreateLocalCharacter();
    
    void DeleteAllCharacters();
    
    void Update(float dt);

    // get
    std::vector<std::unique_ptr<Character>>& getAllCharacters() { return m_characters; }
    Character* getLocalCharacter() { return (m_characters.size() > 0) ? m_characters[0].get() : nullptr; }

    // net
#ifdef USE_SOCKETW
    void handleStreamData(std::vector<RoR::NetRecvPacket> packet);
#endif // USE_SOCKETW
    void UndoRemoteActorCoupling(ActorPtr actor);

private:

    std::vector<std::unique_ptr<Character>> m_characters; // local character is at [0]

    void createRemoteInstance(int sourceid, int streamid);
    void removeStreamSource(int sourceid);

    CacheEntry* fetchCharacterSkin(const std::string& skinName, const std::string& errLogPlayer);
};

/// @} // addtogroup Character
/// @} // addtogroup Gameplay

} // namespace RoR
