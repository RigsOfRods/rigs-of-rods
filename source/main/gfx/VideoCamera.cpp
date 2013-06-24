/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "VideoCamera.h"

#include "MaterialReplacer.h"
#include "ResourceBuffer.h"
#include "Settings.h"
#include "SkyManager.h"
#include "Utils.h"

using namespace Ogre;

int VideoCamera::counter = 0;

VideoCamera::VideoCamera(rig_t *truck) :
	  truck(truck)
	, debugMode(false)
	, debugNode(0)
	, mVidCam()
	, mat()
	, rttTex(0)
	, rwMirror(0)
{
	debugMode = SETTINGS.getBooleanSetting("VideoCameraDebug", false);
}

void VideoCamera::init()
{
	mat = Ogre::MaterialManager::getSingleton().getByName(materialName);

	mVidCam = gEnv->sceneManager->createCamera(materialName + "_camera");

	bool useExternalMirrorWindow = BSETTING("UseVideocameraWindows", false);
	bool fullscreenRW = BSETTING("VideoCameraFullscreen", false);

	// check if this vidcamera is also affected
	if (useExternalMirrorWindow && fullscreenRW)
	{
		int monitor = ISETTING("VideoCameraMonitor_" + TOSTRING(counter), 0);
		if (monitor < 0)
			useExternalMirrorWindow = false;
		// < 0 = fallback to texture
	}

	if (!useExternalMirrorWindow)
	{
		Ogre::TexturePtr rttTexPtr = Ogre::TextureManager::getSingleton().createManual(materialName + "_texture"
			, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			, Ogre::TEX_TYPE_2D
			, mirrorSize.x
			, mirrorSize.y
			, 0 // no mip maps
			, Ogre::PF_R8G8B8
			, Ogre::TU_RENDERTARGET
			, new ResourceBuffer());
		rttTex = rttTexPtr->getBuffer()->getRenderTarget();
		rttTex->setAutoUpdated(false);
	} else
	{
		NameValuePairList misc;
		if (!SSETTING("VideoCameraFSAA", "").empty())
			misc["FSAA"] = SSETTING("VideoCameraFSAA", "");
		
		if (!SSETTING("VideoCameraColourDepth", "").empty())
			misc["colourDepth"] = SSETTING("VideoCameraColourDepth", "");
		else
			misc["colourDepth"] = "32";
		
		if (ISETTING("VideoCameraLeft_" + TOSTRING(counter), 0) > 0)
			misc["left"] = SSETTING("VideoCameraLeft_" + TOSTRING(counter), "");
		
		if (ISETTING("VideoCameraTop_" + TOSTRING(counter), 0) > 0)
			misc["top"] = SSETTING("VideoCameraTop_" + TOSTRING(counter), "");
		if (!SSETTING("VideoCameraWindowBorder", "").empty())
			misc["border"] = SSETTING("VideoCameraWindowBorder", ""); // fixes for windowed mode

		misc["outerDimensions"] = "true"; // fixes for windowed mode

		bool fullscreen = BSETTING("VideoCameraFullscreen", false);
		if (fullscreen)
		{
			int monitor = ISETTING("VideoCameraMonitor_" + TOSTRING(counter), 0);
			misc["monitorIndex"] = TOSTRING(monitor);
		}
		
		rwMirror =  Ogre::Root::getSingleton().createRenderWindow(vidCamName, mirrorSize.x, mirrorSize.y, fullscreen, &misc);
		if (ISETTING("VideoCameraLeft_" + TOSTRING(counter), 0) > 0)
			rwMirror->reposition(ISETTING("VideoCameraLeft_" + TOSTRING(counter), 0), ISETTING("VideoCameraTop_" + TOSTRING(counter), 0));

		if (ISETTING("VideoCameraWidth_" + TOSTRING(counter), 0) > 0)
			rwMirror->resize(ISETTING("VideoCameraWidth_" + TOSTRING(counter), 0), ISETTING("VideoCameraHeight_" + TOSTRING(counter), 0));
		
		rwMirror->setAutoUpdated(false);
		fixRenderWindowIcon(rwMirror);
		rwMirror->setDeactivateOnFocusChange(false);
		// TODO: disable texture mirrors
	}
		
	mVidCam->setNearClipDistance(minclip);
	mVidCam->setFarClipDistance(maxclip);
	mVidCam->setFOVy(Ogre::Degree(fov));
	mVidCam->setAspectRatio((float)mirrorSize.x/(float)mirrorSize.y);

	disabledTexture = mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();
	mat->getTechnique(0)->getPass(0)->setLightingEnabled(false);

	if (rttTex)
	{
		Ogre::Viewport *vp = rttTex->addViewport(mVidCam);
		vp->setClearEveryFrame(true);
		vp->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
		vp->setVisibilityMask(~HIDE_MIRROR);
		vp->setVisibilityMask(~DEPTHMAP_DISABLED);
		vp->setOverlaysEnabled(false);

		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(materialName + "_texture");

		// this is a mirror, flip the image left<>right to have a mirror and not a cameraimage
		if (camRole == 1)
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureUScale (-1);
	}

	if (rwMirror)
	{
		Ogre::Viewport *vp = rwMirror->addViewport(mVidCam);
		vp->setClearEveryFrame(true);
		vp->setBackgroundColour(gEnv->mainCamera->getViewport()->getBackgroundColour());
		vp->setVisibilityMask(~HIDE_MIRROR);
		vp->setVisibilityMask(~DEPTHMAP_DISABLED);
		vp->setOverlaysEnabled(false);
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(disabledTexture);
	}
	
	if (debugMode)
	{
		Entity *ent = gEnv->sceneManager->createEntity("debug-camera.mesh");
		debugNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		ent->setMaterialName("ror-camera");
		debugNode->attachObject(ent);
		debugNode->setScale(0.1,0.1,0.1);
	}
}

void VideoCamera::setActive(bool state)
{
	if (rttTex)
	{
		rttTex->setActive(state);
		if (state)
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(materialName + "_texture");
		else
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(disabledTexture);
	}
	
	if (rwMirror) rwMirror->setActive(state);
}

void VideoCamera::update(float dt)
{
#ifdef USE_CAELUM
	// caelum needs to know that we changed the cameras
	if (gEnv->sky)
		gEnv->sky->notifyCameraChanged(mVidCam);
#endif // USE_CAELUM

	// update the texture now, otherwise shuttering
	if (rttTex) rttTex->update();

	if (rwMirror) rwMirror->update();

	// get the normal of the camera plane now
	Vector3 normal=(-(truck->nodes[nref].smoothpos - truck->nodes[nz].smoothpos)).crossProduct(-(truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos));
	normal.normalise();

	// add user set offset
	Vector3 pos = truck->nodes[camNode].smoothpos +
		(offset.x * normal) +
		(offset.y * (truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos)) +
		(offset.z * (truck->nodes[nref].smoothpos - truck->nodes[nz].smoothpos));

	//avoid the camera roll
	// camup orientates to frustrum of world by default -> rotating the cam related to trucks yaw, lets bind cam rotation videocamera base (nref,ny,nz) as frustum
	// could this be done faster&better with a plane setFrustumExtents ?
	Vector3 frustumUP = truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos;
	frustumUP.normalise();
	mVidCam->setFixedYawAxis(true, frustumUP);

	// camRole 1 = mirror
	if (camRole == 1)
	{
		//rotate the normal of the mirror by user rotation setting so it reflects correct
		normal = rotation * normal;
		// merge camera direction and reflect it on our plane
		mVidCam->setDirection((pos - gEnv->mainCamera->getPosition()).reflect(normal));
	} else
	{
		// this is a videocamera
		if (camRole == -1)
		{
			// rotate the camera according to the nodes orientation and user rotation
			Vector3 refx = truck->nodes[nz].smoothpos - truck->nodes[nref].smoothpos;
			refx.normalise();
			Vector3 refy = truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos;
			refy.normalise();
			Quaternion rot = Quaternion(-refx, -refy, -normal);
			mVidCam->setOrientation(rot * rotation); // rotate the camera orientation towards the calculated cam direction plus user rotation
		} else
		{
			// we assume this is a tracking videocamera
			normal = truck->nodes[lookat].smoothpos - pos;
			normal.normalise();
			Vector3 refx = truck->nodes[nz].smoothpos - truck->nodes[nref].smoothpos;
			refx.normalise();
			// why does this flip ~2-3° around zero orientation and only with trackercam. back to slower crossproduct calc, a bit slower but better .. sigh
			// Vector3 refy = truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos;
			Vector3 refy = refx.crossProduct(normal);
			refy.normalise();
			Quaternion rot = Quaternion(-refx, -refy, -normal);
			mVidCam->setOrientation(rot*rotation); // rotate the camera orientation towards the calculated cam direction plus user rotation
		}
	}

	if (debugMode)
	{
		debugNode->setPosition(pos);
		debugNode->setOrientation(mVidCam->getOrientation());
	}

	// set the new position
	mVidCam->setPosition(pos);
}

VideoCamera *VideoCamera::parseLine(SerializedRig *truck, SerializedRig::parsecontext_t &c)
{
	try
	{
		int nz=-1, ny=-1, nref=-1, ncam=-1, lookto=-1, texx=256, texy=256, crole=-1, cmode=-1;
		float fov=-1.0f, minclip=-1.0f, maxclip=-1.0f, offx=0.0f, offy=0.0f, offz=0.0f, rotx=0.0f, roty=0.0f, rotz=0.0f;
		char materialname[256] = {};
		char vidCamName[256] = {};
		
		Ogre::StringVector args;
		int n = truck->parse_args(c, args, 19);
		nref    = truck->parse_node_number(c, args[0]);
		nz      = truck->parse_node_number(c, args[1]);
		ny      = truck->parse_node_number(c, args[2]);
		ncam    = PARSEINT(args[3]);
		lookto  = PARSEINT(args[4]);
		offx    = PARSEREAL(args[5]);
		offy    = PARSEREAL(args[6]);
		offz    = PARSEREAL(args[7]);
		rotx    = PARSEREAL(args[8]);
		roty    = PARSEREAL(args[9]);
		rotz    = PARSEREAL(args[10]);
		fov     = PARSEREAL(args[11]);
		texx    = PARSEINT (args[12]);
		texy    = PARSEINT (args[13]);
		minclip = PARSEREAL(args[14]);
		maxclip = PARSEREAL(args[15]);
		crole   = PARSEINT (args[16]);
		cmode   = PARSEINT (args[17]);
		strncpy(materialname, args[18].c_str(), 255);
		materialname[255] = '\0';
		if (n > 19)
			strncpy(vidCamName, args[19].c_str(), 255);
		else
			strncpy(vidCamName, materialname, 255); // fallback, use materialname
		
		//if (texx <= 0 || !isPowerOfTwo(texx) || texy <= 0 || !isPowerOfTwo(texy))
		// disabled isPowerOfTwo, as it can be a renderwindow now with custom resolution
		if (texx <= 0 || texy <= 0)
		{
			truck->parser_warning(c, "Wrong texture size definition. trying to continue ...");
			return 0;
		}

		if (minclip < 0 || minclip > maxclip || maxclip < 0)
		{
			truck->parser_warning(c, "Wrong clipping definition. trying to continue ...");
			return 0;
		}

		if (cmode < -2 )
		{
			truck->parser_warning(c, "Camera Mode setting incorrect, trying to continue ...");
			return 0;
		}

		if (crole < -1 || crole >1)
		{
			truck->parser_warning(c, "Camera Role (camera, trace, mirror) setting incorrect, trying to continue ...");
			return 0;
		}

		MaterialPtr mat = MaterialManager::getSingleton().getByName(materialname);
		if (mat.isNull())
		{
			truck->parser_warning(c, "unknown material: '"+String(materialname)+"', trying to continue ...");
			return 0;
		}

		// clone the material to stay unique
		String newMaterialName = String(truck->truckname) + materialname + "_" + TOSTRING(counter++);
		MaterialPtr matNew = mat->clone(newMaterialName);

		// we need to find and replace any materials that could come afterwards
		if (truck && truck->materialReplacer)
			truck->materialReplacer->addMaterialReplace(mat->getName(), newMaterialName);

		VideoCamera *v  = new VideoCamera(truck);
		v->fov          = fov;
		v->minclip      = minclip;
		v->maxclip      = maxclip;
		v->nz           = nz;
		v->ny           = ny;
		v->nref         = nref;
		v->offset       = Vector3(offx, offy, offz);
		v->switchoff    = cmode;            // add performance switch off  ->meeds fix, only "always on" supported yet
		v->materialName = newMaterialName;
		v->vidCamName   = vidCamName;
		v->mirrorSize   = Vector2(texx, texy);

		if (crole != 1)                     //rotate camera picture 180°, skip for mirrors
			rotz += 180;

		v->rotation     = Quaternion(Degree(rotz), Vector3::UNIT_Z) * Quaternion(Degree(roty), Vector3::UNIT_Y) * Quaternion(Degree(rotx), Vector3::UNIT_X);

		if (ncam >= 0)                     // set alternative camposition (optional)
			v->camNode  = ncam;
		else
			v->camNode  = nref;

		if (lookto >= 0)                   // set alternative lookat position (optional)
		{
			v->lookat   = lookto;
			crole       = 0;               // this is a tracecam, overwrite mode setting
		}
		else
			v->lookat   = -1;

		v->camRole      = crole;	        // -1= camera, 0 = trackcam, 1 = mirror

		v->init();

		return v;
	} catch(ParseException &)
	{
		return 0;
	}
}


