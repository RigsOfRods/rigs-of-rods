/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2025 Petr Ohlidal

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

#ifdef USE_SOCKETW

#include "ForwardDeclarations.h"
#include "RoRnet.h"

#include <enet/enet.h>
#include <mutex>
#include <queue>

namespace RoR {

    /// @addtogroup Network
    /// @{

    inline RoRnet::Header* GetRoRnetHeader(ENetPacket* packet)
    {
        ROR_ASSERT(packet);
        ROR_ASSERT(packet->dataLength >= sizeof(RoRnet::Header));
        return (RoRnet::Header*)packet->data;
    }

    inline char* GetRoRnetBuffer(ENetPacket* packet)
    {
        RoRnet::Header* header = GetRoRnetHeader(packet);
        ROR_ASSERT(header->size > 0);
        ROR_ASSERT(packet->dataLength > sizeof(RoRnet::Header));
        return (char*)packet->data + sizeof(RoRnet::Header);
    }

    inline RoRnet::CharacterState* GetRoRnetCharacterState(ENetPacket* packet)
    {
        RoRnet::Header* header = GetRoRnetHeader(packet);
        ROR_ASSERT(header->size == sizeof(RoRnet::CharacterState));
        ROR_ASSERT(packet->dataLength == sizeof(RoRnet::Header) + sizeof(RoRnet::CharacterState));
        return (RoRnet::CharacterState*)(packet->data + sizeof(RoRnet::Header));
    }

    inline RoRnet::ForcesState* GetRoRnetForcesState(ENetPacket* packet)
    {
        RoRnet::Header* header = GetRoRnetHeader(packet);
        ROR_ASSERT(header->size >= sizeof(RoRnet::ForcesState));
        ROR_ASSERT(packet->dataLength >= sizeof(RoRnet::Header) + sizeof(RoRnet::ForcesState));
        return (RoRnet::ForcesState*)(packet->data + sizeof(RoRnet::Header));
    }

    inline bool IsRoRnetDiscardable(int type)
    {
        return type == RoRnet::MSG2_STREAM_DATA_ACTOR || type == RoRnet::MSG2_STREAM_DATA_CHARACTER;
    }

    class ConcurrentPacketQueue
    {
        std::queue<ENetPacket*> m_queue;
        std::mutex m_mutex;
    public:
        void Push(ENetPacket* packet)
        {
            ROR_ASSERT(packet);
            ROR_ASSERT(GetRoRnetHeader(packet));
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(packet);
        }

        ENetPacket* Pop()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty())
            {
                return nullptr;
            }
            ENetPacket* packet = m_queue.front();
            m_queue.pop();
            return packet;
        }
    };

    /// @}   //addtogroup Network

} // namespace RoR

#endif // USE_SOCKETW
