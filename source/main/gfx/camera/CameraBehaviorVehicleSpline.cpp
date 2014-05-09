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
#include "CameraBehaviorVehicleSpline.h"

#include "Beam.h"
#include "Console.h"
#include "InputEngine.h"
#include "Language.h"
#include "Ogre.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicleSpline::CameraBehaviorVehicleSpline() :
	  CameraBehaviorVehicle()
	, numLinkedBeams(0)
	, spline(new SimpleSpline())
	, splineClosed(false)
	, splineLength(1.0f)
	, splineObject(0)
	, splinePos(0.5f)
{
}

void CameraBehaviorVehicleSpline::update(const CameraManager::CameraContext &ctx)
{
	if ( ctx.mCurrTruck->free_camerarail <= 0 )
	{
		gEnv->cameraManager->switchToNextBehavior();
		return;
	}

	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[0]].smoothpos
		- ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[0]].smoothpos).normalisedCopy();

	targetPitch = 0.0f;

	if ( camPitching )
	{
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	if ( ctx.mCurrTruck->getAllLinkedBeams().size() != numLinkedBeams )
	{
		createSpline(ctx);
	}
	updateSpline();
	updateSplineDisplay();

	camLookAt = spline->interpolate(splinePos);

	if ( INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) && INPUTENGINE.isKeyDownValueBounce(OIS::KC_SPACE) )
	{
		autoTracking = !autoTracking;
#ifdef USE_MYGUI
		if ( autoTracking )
		{
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("auto tracking enabled"), "camera_go.png", 3000);
		} else
		{
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("auto tracking disabled"), "camera_go.png", 3000);
		}
#endif // USE_MYGUI
	}

	if ( autoTracking )
	{
		Vector3 centerDir = ctx.mCurrTruck->getPosition() - camLookAt;
		if ( centerDir.length() > 1.0f )
		{
			centerDir.normalise();
			Radian oldTargetDirection = targetDirection;
			targetDirection = -atan2(centerDir.dotProduct(Vector3::UNIT_X), centerDir.dotProduct(-Vector3::UNIT_Z));
			if ( targetDirection.valueRadians() * oldTargetDirection.valueRadians() < 0.0f && centerDir.length() < camDistMin)
			{
				camRotX = -camRotX;
			}
		}
	}

	CameraBehaviorOrbit::update(ctx);
}

bool CameraBehaviorVehicleSpline::mouseMoved(const CameraManager::CameraContext &ctx, const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	if ( INPUTENGINE.isKeyDown(OIS::KC_LCONTROL) && ms.buttonDown(OIS::MB_Right) )
	{
		Real splinePosDiff = ms.X.rel * std::max(0.00005f, splineLength * 0.0000001f);

		if ( INPUTENGINE.isKeyDown(OIS::KC_LSHIFT) || INPUTENGINE.isKeyDown(OIS::KC_RSHIFT) )
		{
			splinePosDiff *= 3.0f;
		}

		if ( INPUTENGINE.isKeyDown(OIS::KC_LMENU) )
		{
			splinePosDiff *= 0.1f;
		}

		splinePos += splinePosDiff;
		
		if ( ms.X.rel > 0 && splinePos > 0.99f )
		{
			if ( splineClosed )
			{
				splinePos -= 1.0f;
			} else
			{
				// u - turn
			}
		} else if ( ms.X.rel < 0 && splinePos < 0.01f )
		{
			if ( splineClosed )
			{
				splinePos += 1.0f;
			} else
			{
				// u - turn
			}
		}

		splinePos  = std::max(0.0f, splinePos);
		splinePos  = std::min(splinePos, 1.0f);

		camRatio = 0.0f;

		return true;
	} else
	{
		camRatio = 5.0f;

		return CameraBehaviorOrbit::mouseMoved(ctx, _arg);
	}
}

void CameraBehaviorVehicleSpline::activate(const CameraManager::CameraContext &ctx, bool reset /* = true */)
{
	if ( !ctx.mCurrTruck || ctx.mCurrTruck->free_camerarail <= 0 )
	{
		gEnv->cameraManager->switchToNextBehavior();
		return;
	} else if ( reset )
	{
		this->reset(ctx);
		createSpline(ctx);
	}
}

void CameraBehaviorVehicleSpline::reset(const CameraManager::CameraContext &ctx)
{
	CameraBehaviorOrbit::reset(ctx);

	camDist = std::min(ctx.mCurrTruck->getMinimalCameraRadius() * 2.0f, 33.0f);
	
	splinePos = 0.5f;
}

void CameraBehaviorVehicleSpline::createSpline(const CameraManager::CameraContext &ctx)
{
	splineClosed = false;
	splineLength = 1.0f;

	spline->clear();
	splineNodes.clear();

	for (int i = 0; i < ctx.mCurrTruck->free_camerarail; i++)
	{
		splineNodes.push_back(&ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameraRail[i]]);
	}

	std::list<Beam*> linkedBeams = ctx.mCurrTruck->getAllLinkedBeams();

	numLinkedBeams = linkedBeams.size();

	if ( numLinkedBeams > 0 )
	{
		for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
		{
			if ( (*it)->free_camerarail <= 0 ) continue;

			Vector3 curSplineFront = splineNodes.front()->AbsPosition;
			Vector3 curSplineBack  = splineNodes.back() ->AbsPosition;

			Vector3 linkedSplineFront = (*it)->nodes[(*it)->cameraRail[0]].AbsPosition;
			Vector3 linkedSplineBack  = (*it)->nodes[(*it)->cameraRail[(*it)->free_camerarail - 1]].AbsPosition;

			if        ( curSplineBack.distance(linkedSplineFront) < 5.0f )
			{
				for (int i = 1; i < (*it)->free_camerarail; i++)
				{
					splineNodes.push_back(&(*it)->nodes[(*it)->cameraRail[i]]);
				}
			} else if ( curSplineFront.distance(linkedSplineFront) < 5.0f )
			{
				for (int i = 1; i < (*it)->free_camerarail; i++)
				{
					splineNodes.push_front(&(*it)->nodes[(*it)->cameraRail[i]]);
				}
			} else if ( curSplineBack.distance(linkedSplineBack) < 5.0f )
			{
				for (int i = (*it)->free_camerarail - 2; i >= 0; i--)
				{
					splineNodes.push_back(&(*it)->nodes[(*it)->cameraRail[i]]);
				}
			} else if ( curSplineFront.distance(linkedSplineBack) < 5.0f )
			{
				for (int i = (*it)->free_camerarail - 2; i >= 0; i--)
				{
					splineNodes.push_front(&(*it)->nodes[(*it)->cameraRail[i]]);
				}
			}
		}
	}

	for (unsigned int i = 0; i < splineNodes.size(); i++)
	{
		spline->addPoint(splineNodes[i]->AbsPosition);
	}

	Vector3 firstPoint = spline->getPoint(0);
	Vector3 lastPoint  = spline->getPoint(spline->getNumPoints() - 1);

	if ( firstPoint.distance(lastPoint) < 1.0f )
	{
		splineClosed = true;
	}

	for (int i = 1; i < spline->getNumPoints(); i++)
	{
		splineLength += spline->getPoint(i - 1).distance(spline->getPoint(i));
	}

	splineLength /= 2.0f;

	if ( !splineObject && ctx.mDebug)
	{
		splineObject = gEnv->sceneManager->createManualObject();
		SceneNode* splineNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

		splineObject->begin("tracks/transred", Ogre::RenderOperation::OT_LINE_STRIP);
		for (int i = 0; i < splineDrawResolution; i++)
		{
			splineObject->position(0, 0, 0);
		}
		splineObject->end();

		splineNode->attachObject(splineObject);
	}
}

void CameraBehaviorVehicleSpline::updateSpline()
{
	for (int i = 0; i < spline->getNumPoints(); i++)
	{
		spline->updatePoint(i, splineNodes[i]->AbsPosition);
	}
}

void CameraBehaviorVehicleSpline::updateSplineDisplay()
{
	if (!splineObject) return;

	splineObject->beginUpdate(0);
	for (int i = 0; i < splineDrawResolution; i++)
	{
		Real parametricDist = i / (float)splineDrawResolution;
		Vector3 position = spline->interpolate(parametricDist);
		splineObject->position(position);
	}
	splineObject->end();
}
