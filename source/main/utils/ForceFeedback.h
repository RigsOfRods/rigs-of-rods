/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal & contributors

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

// Forward decl.
namespace OIS
{
    class ForceFeedback;
    class Effect;
} // namespace OIS

namespace RoR
{

    class ForceFeedback
    {
      public:
        ForceFeedback() : m_device(nullptr), m_hydro_effect(nullptr), m_enabled(false)
        {
        }

        void Setup();
        void SetEnabled(bool v);

        /// we take here :
        /// -roll and pitch inertial forces at the camera: this is not used currently, but it can be used for 2 axes force
        /// feedback devices, like FF joysticks, to render shocks -wheel speed and direction command, for the artificial
        /// auto-centering (which is wheel speed dependant) -hydro beam stress, the ideal data source for FF wheels
        void SetForces(float roll, float pitch, float wspeed, float dircommand, float stress);

      private:
        OIS::ForceFeedback *m_device;
        OIS::Effect *       m_hydro_effect;
        bool                m_enabled; /// Disables FF when not in vehicle
    };

} // namespace RoR
