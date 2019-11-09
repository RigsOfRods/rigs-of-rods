/*
--------------------------------------------------------------------------------
This source file is part of Hydrax.
Visit http://www.ogre3d.org/tikiwiki/Hydrax

Copyright (C) 2011 Jose Luis Cercós Pita <jlcercos@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place - Suite 330, Boston, MA 02111-1307, USA, or go to
http://www.gnu.org/copyleft/lesser.txt.
--------------------------------------------------------------------------------
*/

// ----------------------------------------------------------------------------
// Include the main header
// ----------------------------------------------------------------------------
#include <Real.h>

// ----------------------------------------------------------------------------
// Include Hydrax
// ----------------------------------------------------------------------------
#include <Hydrax.h>

#define _def_PackedNoise true

namespace Hydrax
{
    namespace Noise
    {

        Real::Real() : Noise("Real", true), mTime(0), mPerlinNoise(0), mGPUNormalMapManager(0)
        {
            mPerlinNoise = new Perlin(/*Generic one*/);
        }

        Real::~Real()
        {
            remove();
            delete mPerlinNoise;

            HydraxLOG(getName() + " destroyed.");
        }

        void Real::create()
        {
            if (isCreated()) { return; }

            Noise::create();
            mPerlinNoise->create();
        }

        void Real::remove()
        {
            if (areGPUNormalMapResourcesCreated()) { Noise::removeGPUNormalMapResources(mGPUNormalMapManager); }

            if (!isCreated()) { return; }

            mTime = 0;

            mPerlinNoise->remove();
            mWaves.clear();
            mPressurePoints.clear();
            Noise::remove();
        }

        int Real::addWave(Ogre::Vector2 dir, float A, float T, float p)
        {
            mWaves.push_back(Wave(dir, A, T, p));
            return static_cast<int>(mWaves.size());
        }

        int Real::addPressurePoint(Ogre::Vector2 Orig, float p, float T, float L)
        {
            mPressurePoints.push_back(PressurePoint(Orig, p, T, L));
            return static_cast<int>(mPressurePoints.size());
        }

        Wave Real::getWave(int id) const
        {
            return mWaves.at(id);
        }

        bool Real::eraseWave(int id)
        {
            if ((id < 0) || (id >= (int)mWaves.size()))
            {
                HydraxLOG("Error (Real::eraseWave):\tCan't delete the wave.");
                HydraxLOG("\tIdentifier exceed the waves vector dimensions.");
                return false;
            }
            size_t Size = mWaves.size();
            mWaves.erase(mWaves.begin() + id);
            if (mWaves.size() == Size)
            {
                HydraxLOG("Error (Real::eraseWave):\tCan't delete the wave.");
                HydraxLOG("\tThe size before deletion matchs with the size after the operation.");
                return false;
            }
            return true;
        }

        bool Real::modifyWave(int id, Ogre::Vector2 dir, float A, float T, float p)
        {
            if ((id < 0) || (id >= (int)mWaves.size()))
            {
                HydraxLOG("Error (Real::eraseWave):\tCan't delete the wave.");
                HydraxLOG("\tIdentifier exceed the waves vector dimensions.");
                return false;
            }
            mWaves.at(id) = Wave(dir, A, T, p);
            return true;
        }

        bool Real::createGPUNormalMapResources(GPUNormalMapManager *g)
        {
            /// @todo Create gpuNormal creator.
            return false;
        }

        void Real::saveCfg(Ogre::String &Data)
        {
            mPerlinNoise->saveCfg(Data);
        }

        bool Real::loadCfg(Ogre::ConfigFile &CfgFile)
        {
            if (!Noise::loadCfg(CfgFile)) { return false; }

            HydraxLOG("\tReading options...");
            mPerlinNoise->setOptions(Perlin::Options(CfgFileManager::_getIntValue(CfgFile, "Perlin_Octaves"),
                                                     CfgFileManager::_getFloatValue(CfgFile, "Perlin_Scale"),
                                                     CfgFileManager::_getFloatValue(CfgFile, "Perlin_Falloff"),
                                                     CfgFileManager::_getFloatValue(CfgFile, "Perlin_Animspeed"),
                                                     CfgFileManager::_getFloatValue(CfgFile, "Perlin_Timemulti"),
                                                     CfgFileManager::_getFloatValue(CfgFile, "Perlin_GPU_Strength"),
                                                     CfgFileManager::_getVector3Value(CfgFile, "Perlin_GPU_LODParameters")));
            HydraxLOG("\tOptions readed.");

            return true;
        }

        void Real::update(const Ogre::Real &timeSinceLastFrame)
        {
            int i;
            mTime += timeSinceLastFrame;
            mPerlinNoise->update(timeSinceLastFrame);
            for (i = (int)mWaves.size() - 1; i >= 0; i--)
            {
                mWaves.at(i).update(timeSinceLastFrame);
            }
            for (i = (int)mPressurePoints.size() - 1; i >= 0; i--)
            {
                if (!mPressurePoints.at(i).update(timeSinceLastFrame)) { mPressurePoints.erase(mPressurePoints.begin() + i); }
            }
        }

        float Real::getValue(const float &x, const float &y)
        {
            int i;
            /// 1st.- Perlin height
            float H = mPerlinNoise->getValue(x, y);
            /// 2nd.- Waves height
            for (i = 0; i < (int)mWaves.size(); i++)
            {
                H += mWaves.at(i).getValue(x, y);
            }
            /// 3rd.- Pressure points height
            for (i = 0; i < (int)mPressurePoints.size(); i++)
            {
                H += mPressurePoints.at(i).getValue(x, y);
            }
            return H;
        }

    } // namespace Noise
} // namespace Hydrax
