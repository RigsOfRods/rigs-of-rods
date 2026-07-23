// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.


#include "CaelumPort/PointStarfield.h"
#include "CaelumPort/CaelumExceptions.h"
#include "CaelumPort/Astronomy.h"
#include "CaelumPort/InternalUtilities.h"

using namespace Ogre;

namespace CaelumPort
{
    const Ogre::String PointStarfield::STARFIELD_MATERIAL_NAME = "CaelumPort/StarPoint";

    const Ogre::String PointStarfield::STARFIELD_MANUALOBJECT_NAME = "CaelumPort/PointStarfield";

    PointStarfield::PointStarfield (
            Ogre::SceneManager *sceneMgr,
            Ogre::SceneNode *caelumRootNode,
            bool initWithCatalogue):
        mSceneMgr(sceneMgr)
    {
        mMag0PixelSize = 16;
        mMinPixelSize = 4;
        mMaxPixelSize = 6;
        mMagnitudeScale = Math::Pow(100, 0.2);
        mObserverLatitude = 45;
        mObserverLongitude = 0;

        String uniqueSuffix = "/" + InternalUtilities::pointerToString(this);

        // Load material.
        mMaterial = InternalUtilities::checkLoadMaterialClone(
                    STARFIELD_MATERIAL_NAME,
                    STARFIELD_MATERIAL_NAME + uniqueSuffix);

        mParams.setup(mMaterial->getTechnique(0)->getPass(0)->getVertexProgramParameters());

        // We use a separate data source.
        Ogre::String objName = STARFIELD_MANUALOBJECT_NAME + uniqueSuffix;
        mManualObj= (sceneMgr->createManualObject (objName));
        mManualObj->setDynamic(false);
        mManualObj->setRenderQueueGroup (CAELUM_RENDER_QUEUE_STARFIELD);
        sceneMgr->getRenderQueue()->getQueueGroup(CAELUM_RENDER_QUEUE_STARFIELD)->setShadowsEnabled (false);
        mManualObj->setCastShadows(false);

        mNode= (caelumRootNode->createChildSceneNode ());
        mNode->attachObject (mManualObj);

        if (initWithCatalogue) {
            addBrightStarCatalogue ();
        }
    }

    PointStarfield::~PointStarfield ()
    {
            // Detach and destroy manual object.
            mNode->detachObject (mManualObj);
            mSceneMgr->destroyManualObject (mManualObj);

            mSceneMgr->destroySceneNode(mNode);
    
            // Destroy material.
            mMaterial.reset();
    }

    void PointStarfield::notifyStarVectorChanged () {
        invalidateGeometry ();
    }

    void PointStarfield::clearAllStars () {
        mStars.clear();
        notifyStarVectorChanged ();
    }

    Real randReal () {
        return rand() / static_cast<float>(RAND_MAX);
    }

    Real randReal (Real min, Real max) {
        Real f = randReal ();
        return min * (1 - f) + max * f;
    }

    void PointStarfield::addRandomStars (int count)
    {
        for (int i = 0; i < count; ++i) {
            // Generate a vector inside a sphere
            Ogre::Vector3 pos;
            do {
                pos.x = randReal(-1, 1);
                pos.y = randReal(-1, 1);
                pos.z = randReal(-1, 1);
            } while (pos.squaredLength () >= 1);

            // Convert to rasc/decl angles.
            LongReal rasc, decl, dist;
            Astronomy::convertRectangularToSpherical(
                    pos.x, pos.y, pos.z,
                    rasc, decl, dist);

            Star s;
            s.RightAscension = Ogre::Degree (rasc);
            s.Declination = Ogre::Degree (decl);
            // This distribution is wrong.
            s.Magnitude = 6 * pos.squaredLength () + 1.5;
            mStars.push_back(s);
        }
        notifyStarVectorChanged ();
    }

    void PointStarfield::addStar (const BrightStarCatalogueEntry &entry) {
        Star s;
        s.RightAscension = Ogre::Degree(360 / 24.0f * (
                Math::Abs(entry.rasc_hour) +
                entry.rasc_min / 60.0f +
                entry.rasc_sec / 3600.0f));
        s.Declination = Ogre::Degree(Math::Sign(entry.decl_deg) * (
                Math::Abs(entry.decl_deg) +
                entry.decl_min / 60.0f +
                entry.decl_sec / 3600.0f));
        s.Magnitude = entry.magn;
        mStars.push_back(s);

        notifyStarVectorChanged ();
    }

    void PointStarfield::addBrightStarCatalogue (int count) {
        assert(count >= 0);
        if (count < BrightStarCatalogueSize) {
            // Only sort if we don't add everything.
            // It would be lovely if the catalogue was already sorted.
            std::vector<std::pair<Real, int> > vec;
            vec.reserve(BrightStarCatalogueSize);
            for (int i = 0; i < BrightStarCatalogueSize; ++i) {
                vec.push_back(std::make_pair(BrightStarCatalogue[i].magn, i));
            }
            sort(vec.begin(), vec.end());
            for (int i = 0; i < count; ++i) {
                addStar(BrightStarCatalogue[vec[i].second]);
            }
        } else {
            assert(count == BrightStarCatalogueSize);
            for (int i = 0; i < BrightStarCatalogueSize; ++i) {
                addStar(BrightStarCatalogue[i]);
            }
        }
        notifyStarVectorChanged ();
    }

    void PointStarfield::invalidateGeometry () {
        mValidGeometry = false;
    }

    void PointStarfield::ensureGeometry ()
    {
        if (mValidGeometry) {
            return;
        }

        //Ogre::LogManager::getSingleton ().logMessage ("CaelumPort: Recomputing starfield geometry.");

        size_t starCount = mStars.size();

        mManualObj->clear();
        mManualObj->estimateVertexCount(6 * starCount);
        mManualObj->begin(mMaterial->getName (), Ogre::RenderOperation::OT_TRIANGLE_LIST, mMaterial->getGroup());
        for (uint i = 0; i < starCount; ++i)
        {
            const Star& star = mStars[i];

            // North celestial pole is at +Y, vernal equinox at -X
            Ogre::Vector3 pos;
            pos.z =  Math::Sin(star.RightAscension) * Math::Cos(star.Declination);
            pos.x = -Math::Cos(star.RightAscension) * Math::Cos(star.Declination);
            pos.y =  Math::Sin(star.Declination);

            //mManualObj->colour (Ogre::ColourValue::White);
            mManualObj->position (pos);
            mManualObj->textureCoord (+1, -1, star.Magnitude);
            mManualObj->position (pos);
            mManualObj->textureCoord (+1, +1, star.Magnitude);
            mManualObj->position (pos);
            mManualObj->textureCoord (-1, -1, star.Magnitude);

            mManualObj->position (pos);
            mManualObj->textureCoord (-1, -1, star.Magnitude);
            mManualObj->position (pos);
            mManualObj->textureCoord (+1, +1, star.Magnitude);
            mManualObj->position (pos);
            mManualObj->textureCoord (-1, +1, star.Magnitude);
        }
        mManualObj->end();

        // Set finite bounds on the starfield to avoid parent AABB infection
        AxisAlignedBox box(Ogre::AxisAlignedBox::EXTENT_FINITE);
        mManualObj->setBoundingBox (box);

        mValidGeometry = true;
    }
    
    void PointStarfield::Params::setup(Ogre::GpuProgramParametersSharedPtr vpParams)
    {
        this->vpParams = vpParams;
        this->mag_scale.bind(vpParams, "mag_scale");
        this->mag0_size.bind(vpParams, "mag0_size");
        this->min_size.bind(vpParams, "min_size");
        this->max_size.bind(vpParams, "max_size");
        this->aspect_ratio.bind(vpParams, "aspect_ratio");
    }

    void PointStarfield::notifyCameraChanged (Ogre::Camera *cam) {
        CameraBoundElement::notifyCameraChanged (cam);

        // Shader params are changed for every camera.
        Pass* pass = mMaterial->getBestTechnique ()->getPass (0);
        GpuProgramParametersSharedPtr fpParams = pass->getFragmentProgramParameters ();
        GpuProgramParametersSharedPtr vpParams = pass->getVertexProgramParameters ();

        int height = cam->getViewport ()-> getActualHeight ();
        int width = cam->getViewport ()-> getActualWidth ();
        Real pixFactor = 1.0f / width;
        Real magScale = -Math::Log (mMagnitudeScale) / 2;
        Real mag0Size = mMag0PixelSize * pixFactor;
        Real minSize = mMinPixelSize * pixFactor;
        Real maxSize = mMaxPixelSize * pixFactor;
        Real aspectRatio = static_cast<Real>(width) / height;

        // These params are relative to the size of the screen.
        mParams.mag_scale.set(mParams.vpParams, magScale);
        mParams.mag0_size.set(mParams.vpParams, mag0Size);
        mParams.min_size.set(mParams.vpParams, minSize);
        mParams.max_size.set(mParams.vpParams, maxSize);
        mParams.aspect_ratio.set(mParams.vpParams, aspectRatio);
    }

    void PointStarfield::setFarRadius (Ogre::Real radius) {
        CameraBoundElement::setFarRadius(radius);
        mNode->setScale (Ogre::Vector3::UNIT_SCALE * radius);
    }

    void PointStarfield::update (LongReal julDay) {
        LongReal vernalEquinoxHourAngle;
        Astronomy::getVernalEquinoxHourAngle(julDay, mObserverLongitude.valueDegrees(), vernalEquinoxHourAngle);
        Ogre::Quaternion orientation = 
            Ogre::Quaternion (Ogre::Radian (-mObserverLatitude + Ogre::Degree (90)), Ogre::Vector3::UNIT_X) *
            Ogre::Quaternion(-Ogre::Degree(vernalEquinoxHourAngle + 90.0), Ogre::Vector3::UNIT_Y );
        mNode->setOrientation (orientation);
        ensureGeometry ();
    }
}
