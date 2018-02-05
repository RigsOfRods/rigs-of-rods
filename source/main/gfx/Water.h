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

#include "IWater.h"
#include "RoRPrerequisites.h"

#include <OgreHardwareVertexBuffer.h> // Ogre::HardwareVertexBufferSharedPtr
#include <OgreMesh.h>
#include <OgrePrerequisites.h> // Ogre::TexturePtr
#include <OgreTexture.h>
#include <OgreVector3.h>
#include <vector>

class Water : public IWater, public ZeroedMemoryAllocator
{
public:

    Water();
    ~Water();

    float GetStaticWaterHeight();
    float getHeightWaves(Ogre::Vector3 pos);
    Ogre::Vector3 getVelocity(Ogre::Vector3 pos);

    void setCamera(Ogre::Camera* cam);
    void setHeight(float value);
    void setSunPosition(Ogre::Vector3);
    void setVisible(bool value);

    bool isUnderWater(Ogre::Vector3 pos);
    bool allowUnderWater();
    void moveTo(float centerheight);
    void prepareShutdown();
    void showWave(Ogre::Vector3 refpos);
    void update();
    void framestep(float dt);
    void updateReflectionPlane(float h);
    bool isCameraUnderWater();

    void processWater();

private:

    float getWaveHeight(Ogre::Vector3 pos);

    struct WaveTrain
    {
        float amplitude;
        float maxheight;
        float wavelength;
        float wavespeed;
        float direction;
        float dir_sin;
        float dir_cos;
    };

    static const int WAVEREZ = 100;

    bool visible;
    float* wbuffer;
    float wHeight, orgHeight, wbHeight;
    float maxampl;
    float mScale;
    int framecounter;
    bool ForceUpdate;

    Ogre::MeshPtr mprt;
    Ogre::Vector3 mapSize;
    Ogre::Camera* mRenderCamera;
    Ogre::Camera* mReflectCam;
    Ogre::Camera* mRefractCam;
    Ogre::HardwareVertexBufferSharedPtr wbuf;
    Ogre::RenderTexture* rttTex1;
    Ogre::RenderTexture* rttTex2;
    Ogre::TexturePtr     m_refraction_rtt_texture;
    Ogre::TexturePtr     m_reflection_rtt_texture;
    Ogre::SceneNode* pBottomNode;
    Ogre::SceneNode* pWaterNode;
    Ogre::Viewport *vRtt1, *vRtt2;
    std::vector<WaveTrain> m_wavetrain_defs;
};
