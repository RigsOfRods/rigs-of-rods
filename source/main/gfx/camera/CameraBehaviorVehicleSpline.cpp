/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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

#include "CameraBehaviorVehicleSpline.h"

#include <Ogre.h>

#include "Application.h"
#include "Beam.h"
#include "GUI_GameConsole.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "Language.h"
#include "Settings.h"

using namespace Ogre;
using namespace RoR;

CameraBehaviorVehicleSpline::CameraBehaviorVehicleSpline() :
    
     numLinkedBeams(0)
    , spline(new SimpleSpline())
    , splineClosed(false)
    , splineLength(1.0f)
    , splineObject(0)
    , splinePos(0.5f)
{
}

CameraBehaviorVehicleSpline::~CameraBehaviorVehicleSpline()
{
    if (spline)
        delete spline;
    if (splineObject)
        delete splineObject;
}

void CameraBehaviorVehicleSpline::update( CameraManager::CameraContext& ctx)
{
    if (ctx.cct_player_actor->ar_num_camera_rails <= 0)
    {
        gEnv->cameraManager->switchToNextBehavior();
        return;
    }

    Vector3 dir = ctx.cct_player_actor->getDirection();

    ctx.targetPitch = 0.0f;

    if (App::gfx_extcam_mode.GetActive() == GfxExtCamMode::PITCHING)
    {
        ctx.targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
    }

    if (ctx.cct_player_actor->GetAllLinkedActors().size() != numLinkedBeams)
    {
        createSpline(ctx);
    }
    updateSpline();
    updateSplineDisplay();

    ctx.camLookAt = spline->interpolate(splinePos);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) && RoR::App::GetInputEngine()->isKeyDownValueBounce(OIS::KC_SPACE))
    {
        autoTracking = !autoTracking;
        if (autoTracking)
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Auto tracking enabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Auto tracking enabled"));
        }
        else
        {
            RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Auto tracking disabled"), "camera_go.png", 3000);
            RoR::App::GetGuiManager()->PushNotification("Notice:", _L("Auto tracking disabled"));
        }
    }

    if (autoTracking)
    {
        Vector3 centerDir = ctx.cct_player_actor->getPosition() - ctx.camLookAt;
        if (centerDir.length() > 1.0f)
        {
            centerDir.normalise();
            Radian oldTargetDirection = ctx.targetDirection;
            ctx.targetDirection = -atan2(centerDir.dotProduct(Vector3::UNIT_X), centerDir.dotProduct(-Vector3::UNIT_Z));
            if (ctx.targetDirection.valueRadians() * oldTargetDirection.valueRadians() < 0.0f && centerDir.length() < ctx.camDistMin)
            {
                ctx.camRotX = -ctx.camRotX;
            }
        }
    }

    CameraManager::CameraBehaviorOrbitUpdate(ctx);
}

bool CameraBehaviorVehicleSpline::mouseMoved( CameraManager::CameraContext& ctx, const OIS::MouseEvent& _arg)
{
    const OIS::MouseState ms = _arg.state;

    ctx.camRatio = 1.0f / (ctx.cct_dt * 4.0f);

    if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LCONTROL) && ms.buttonDown(OIS::MB_Right))
    {
        Real splinePosDiff = ms.X.rel * std::max(0.00005f, splineLength * 0.0000001f);

        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LSHIFT) || RoR::App::GetInputEngine()->isKeyDown(OIS::KC_RSHIFT))
        {
            splinePosDiff *= 3.0f;
        }

        if (RoR::App::GetInputEngine()->isKeyDown(OIS::KC_LMENU))
        {
            splinePosDiff *= 0.1f;
        }

        splinePos += splinePosDiff;

        if (ms.X.rel > 0 && splinePos > 0.99f)
        {
            if (splineClosed)
            {
                splinePos -= 1.0f;
            }
            else
            {
                // u - turn
            }
        }
        else if (ms.X.rel < 0 && splinePos < 0.01f)
        {
            if (splineClosed)
            {
                splinePos += 1.0f;
            }
            else
            {
                // u - turn
            }
        }

        splinePos = std::max(0.0f, splinePos);
        splinePos = std::min(splinePos, 1.0f);

        return true;
    }
    else
    {
        return CameraManager::CameraBehaviorOrbitMouseMoved(ctx, _arg);
    }
}

void CameraBehaviorVehicleSpline::reset( CameraManager::CameraContext& ctx)
{
    CameraManager::CameraBehaviorOrbitReset(ctx);

    ctx.camDist = std::min(ctx.cct_player_actor->getMinimalCameraRadius() * 2.0f, 33.0f);

    splinePos = 0.5f;
}

void CameraBehaviorVehicleSpline::createSpline( CameraManager::CameraContext& ctx)
{
    splineClosed = false;
    splineLength = 1.0f;

    spline->clear();
    splineNodes.clear();

    for (int i = 0; i < ctx.cct_player_actor->ar_num_camera_rails; i++)
    {
        splineNodes.push_back(&ctx.cct_player_actor->ar_nodes[ctx.cct_player_actor->ar_camera_rail[i]]);
    }

    std::list<Actor*> linkedBeams = ctx.cct_player_actor->GetAllLinkedActors();

    numLinkedBeams = linkedBeams.size();

    if (numLinkedBeams > 0)
    {
        for (std::list<Actor*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
        {
            if ((*it)->ar_num_camera_rails <= 0)
                continue;

            Vector3 curSplineFront = splineNodes.front()->AbsPosition;
            Vector3 curSplineBack = splineNodes.back()->AbsPosition;

            Vector3 linkedSplineFront = (*it)->ar_nodes[(*it)->ar_camera_rail[0]].AbsPosition;
            Vector3 linkedSplineBack = (*it)->ar_nodes[(*it)->ar_camera_rail[(*it)->ar_num_camera_rails - 1]].AbsPosition;

            if (curSplineBack.distance(linkedSplineFront) < 5.0f)
            {
                for (int i = 1; i < (*it)->ar_num_camera_rails; i++)
                {
                    splineNodes.push_back(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
            else if (curSplineFront.distance(linkedSplineFront) < 5.0f)
            {
                for (int i = 1; i < (*it)->ar_num_camera_rails; i++)
                {
                    splineNodes.push_front(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
            else if (curSplineBack.distance(linkedSplineBack) < 5.0f)
            {
                for (int i = (*it)->ar_num_camera_rails - 2; i >= 0; i--)
                {
                    splineNodes.push_back(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
            else if (curSplineFront.distance(linkedSplineBack) < 5.0f)
            {
                for (int i = (*it)->ar_num_camera_rails - 2; i >= 0; i--)
                {
                    splineNodes.push_front(&(*it)->ar_nodes[(*it)->ar_camera_rail[i]]);
                }
            }
        }
    }

    for (unsigned int i = 0; i < splineNodes.size(); i++)
    {
        spline->addPoint(splineNodes[i]->AbsPosition);
    }

    Vector3 firstPoint = spline->getPoint(0);
    Vector3 lastPoint = spline->getPoint(spline->getNumPoints() - 1);

    if (firstPoint.distance(lastPoint) < 1.0f)
    {
        splineClosed = true;
    }

    for (int i = 1; i < spline->getNumPoints(); i++)
    {
        splineLength += spline->getPoint(i - 1).distance(spline->getPoint(i));
    }

    splineLength /= 2.0f;

    if (!splineObject && ctx.cct_debug)
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
    if (!splineObject)
        return;

    splineObject->beginUpdate(0);
    for (int i = 0; i < splineDrawResolution; i++)
    {
        Real parametricDist = i / (float)splineDrawResolution;
        Vector3 position = spline->interpolate(parametricDist);
        splineObject->position(position);
    }
    splineObject->end();
}
