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

#pragma once

#include "Application.h"

/// @file
/// @version 1
/// @brief torquecurve loader.
/// @authors flypiper
/// @authors Thomas Fischer (thomas{AT}rigsofrods{DOT}com)

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Trucks
/// @{

/**
 *  @brief This class loads and processes a torque curve for a vehicle.
 */
class TorqueCurve
{
public:
    const static Ogre::String customModel;

    TorqueCurve(); //!< Constructor
    ~TorqueCurve(); //!< Destructor

    /**
     * Returns the calculated engine torque based on the given RPM, interpolating the torque curve spline.
     * @param The current engine RPM.
     * @return Calculated engine torque.
     */
    Ogre::Real getEngineTorque(Ogre::Real rpm);

    /**
     * Sets the torque model which is used for the vehicle.
     * @param name name of the torque model which should be used.
     * @return 0 on success, 1 if torque model is not found.
     */
    int setTorqueModel(Ogre::String name);

    /**
    * Creates new torque curve.
    * @return True if created, false if already existed.
    */
    bool CreateNewCurve(Ogre::String const& name = customModel);

    /**
     * Adds a point to the torque curve graph.
     * @param progress 0 - 1
     * @param model Torque model name (i.e. 'turbodiesel').
     */
    void AddCurveSample(float rpm, float progress, Ogre::String const& model = customModel);

    /**
     * Returns the used spline.
     * @return The torque spline used by the vehicle.
     */
    Ogre::SimpleSpline* getUsedSpline() { return usedSpline; };

    /**
     * Returns the name of the torque model used by the vehicle.
     * @return The name of the torque model used by the vehicle.
     */
    Ogre::String getTorqueModel() { return usedModel; };

    /**
     * Spaces the points of a spline evenly; this is needed for the correct calculation of the Ogre simple spline.
     * @param spline Pointer to the spline which should be processed.
     * @return 0 on success, 1 on error
     */
    int spaceCurveEvenly(Ogre::SimpleSpline* spline);

protected:

    /**
     * Loads default torque models from the 'torque_models.cfg' file.
     * @return 0 on success, 1 if 'torque_model.cfg' file not found.
     */
    int loadDefaultTorqueModels();

    /**
     * Processes the given vector.
     * Adds points to a torque curve spline; or if a new model is found, generating a new spline.
     * @param args Vector of arguments of the line which should be processed.
     * @param model Torque model name (i.e. 'turbodiesel')
     * @return setTorqueModel() called if one argument given, 1 on error, 0 on success
     */
    int processLine(Ogre::StringVector args, Ogre::String model);

    Ogre::SimpleSpline* usedSpline; //!< spline which is used for calculating the torque, set by setTorqueModel().
    Ogre::String usedModel; //!< name of the torque model used by the truck.
    std::map<Ogre::String, Ogre::SimpleSpline> splines; //!< container were all torque curve splines are stored in.
};

/// @} // addtogroup Trucks
/// @} // addtogroup Gameplay

} // namespace RoR
