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

#pragma once


// Backend
#include "Application.h"
#include "GameContext.h"

#include <mutex>

/// @addtogroup Frontend
/// @{

class MessageHandler: public RoR::MessageListener
{
public:
    virtual bool ProcessMessage(RoR::Message& m) override;
    bool WasExitRequested();
private:
    void SetExitRequested();

    bool m_exit_requested = false;
    std::mutex m_exit_mutex;
};

/// @} // addtogroup Frontend


