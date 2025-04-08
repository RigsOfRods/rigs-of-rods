/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2014 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
    @file   ErrorUtils.h
    @author Thomas Fischer
    @date   3rd October 2009
*/

#pragma once

#include "Application.h"

/// @addtogroup Application
/// @{

struct ErrorUtils
{
    /**
     * shows a simple error message box
     * @return 0 on success, everything else on error
     */
    static int ShowError(const std::string& title, const std::string& message);

    /**
     * shows a simple info message box
     * @return 0 on success, everything else on error
     */
    static int ShowInfo(const std::string& title, const std::string& message);

    /**
     * shows a generic message box
     * @param type 0 for error, 1 for info
     * @return 0 on success, everything else on error
     */
    static int ShowMsgBox(const std::string& title, const std::string& err, int type);
};

/// @} // addtogroup Application
